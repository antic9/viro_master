//
//  VROApiKeyValidatorDynamo.m
//  ViroRenderer
//
//  Created by Andy Chu on 10/18/16.
//  Copyright © 2016 Viro Media. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining
//  a copy of this software and associated documentation files (the
//  "Software"), to deal in the Software without restriction, including
//  without limitation the rights to use, copy, modify, merge, publish,
//  distribute, sublicense, and/or sell copies of the Software, and to
//  permit persons to whom the Software is furnished to do so, subject to
//  the following conditions:
//
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#import "VROApiKeyValidatorDynamo.h"
#import "VROApiKey.h"
#import "VROApiKeyMetrics.h"
#import "VROApiMetricsIncrementRequest.h"
#import "VROCredentials.h"
#import <AWSDynamoDB/AWSDynamoDB.h>

static NSInteger const kVROApiValidatorMinRetryDelay = 2; // seconds
static NSInteger const kVROApiValidatorMaxRetryDelay = 64; // seconds

@interface VROApiKeyValidatorDynamo()

/**
 This is the latest key that we've been given
 */
@property (nonatomic, copy) NSString *bundleId;
@property (nonatomic, readwrite) AWSDynamoDBObjectMapper *dynamoObjectMapper;

/**
 This object locks any changes to the 3 objects below it.
 */
@property (nonatomic, strong) NSObject *taskLock;
@property (nonatomic, copy) NSString *currentKeyToCheck;
@property (nonatomic, copy) NSString *nextKeyToCheck;
@property (nonatomic, strong) VROApiKeyValidatorBlock nextTaskCompletion;

@end

@implementation VROApiKeyValidatorDynamo

- (id)init {
    self = [super init];
    if (self) {
        _taskLock = [[NSObject alloc] init];
        _bundleId = [[NSBundle mainBundle] bundleIdentifier];

#if DEBUG
        [AWSDDLog sharedInstance].logLevel = AWSLogLevelWarn;
#else
        [AWSDDLog sharedInstance].logLevel = AWSLogLevelWarn;
#endif
        NSString *encodedAccessKey = kVROEncodedAccessKey;
        NSString *encodedSecretKey = kVROEncodedSecretKey;
      
        NSData *accessKeyData = [[NSData alloc] initWithBase64EncodedString:encodedAccessKey options:0];
        NSData *secretKeyData = [[NSData alloc] initWithBase64EncodedString:encodedSecretKey options:0];

        NSString *accessKey = [[NSString alloc] initWithData:accessKeyData encoding:NSUTF8StringEncoding];
        NSString *secretKey = [[NSString alloc] initWithData:secretKeyData encoding:NSUTF8StringEncoding];

        AWSStaticCredentialsProvider *credentialsProvider = [[AWSStaticCredentialsProvider alloc] initWithAccessKey:accessKey
                                                                                                          secretKey:secretKey];
        AWSServiceConfiguration *configuration = [[AWSServiceConfiguration alloc] initWithRegion:AWSRegionUSWest2 credentialsProvider:credentialsProvider];
        AWSServiceManager.defaultServiceManager.defaultServiceConfiguration = configuration;
        _dynamoObjectMapper = [AWSDynamoDBObjectMapper defaultDynamoDBObjectMapper];
    }
    return self;
}

/**
 This function validates the given apiKey.
 */
- (void)validateApiKey:(NSString *)apiKey platform:(NSString *)platform withCompletionBlock:(VROApiKeyValidatorBlock)completionBlock {
  
    @synchronized (self.taskLock) {
        if (apiKey == nil) {
            completionBlock(NO);
            return;
        }

        // If there's already a key in the process of being checked (currentKeyToCheck), then store it in nextKeyToCheck
        // this is there doesn't seem to be a way to cancel an ongoing AWSTask.
        if (self.currentKeyToCheck) {
            self.nextKeyToCheck = apiKey;
            self.nextTaskCompletion = completionBlock;
        } else {
            self.currentKeyToCheck = apiKey;
            [self checkCurrentKey:completionBlock platform:platform attempt:1];
        }
    }
}

/**
 This function checks the current key by creating a task and executing it.
 */
- (void)checkCurrentKey:(VROApiKeyValidatorBlock)completionBlock platform:(NSString *)platform attempt:(NSInteger)attempt {
  
    NSInteger delay = attempt == 1 ? 0 : MIN(kVROApiValidatorMaxRetryDelay, pow(kVROApiValidatorMinRetryDelay, attempt - 1));

    NSLog(@"[ApiKeyValidator] Attempt %ld, performing validation in %ld seconds", (long)attempt, (long)delay);
    dispatch_time_t delayTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delay * NSEC_PER_SEC));
    dispatch_after(delayTime, dispatch_get_main_queue(), ^(void) {
        // construct a NEW task and run it. We can't keep calling continueWithBlock on a task esp if it already failed.
        AWSTask *task = [self.dynamoObjectMapper load:[VROApiKey class] hashKey:self.currentKeyToCheck rangeKey:nil];
        [task continueWithBlock:[self getValidationEndBlock:completionBlock platform:platform attempt:attempt]];
    });
}

/**
 This function returns an AWSContinuationBlock which gets called with the result of the
 attempt to get the apiKey from DynamoDB. It should always be given the number of the attempt
 it is registered to (NOT the next one).
 */
- (AWSContinuationBlock)getValidationEndBlock:(VROApiKeyValidatorBlock)completionBlock platform:(NSString *)platform attempt:(NSInteger)attempt {
    return ^id(AWSTask *task) {
        @synchronized (self.taskLock) {
            // If there's a next task, just start running it, don't even bother to call the completion block.
            if (self.nextKeyToCheck) {
                self.currentKeyToCheck = self.nextKeyToCheck;
                [self checkCurrentKey:self.nextTaskCompletion platform:platform attempt:attempt + 1];
                self.nextKeyToCheck = nil;
                self.nextTaskCompletion = nil;
                return nil;
            }
          
            // If the task is cancelled or has errored out, then try to get the apiKey again.
            if (task.cancelled || task.faulted) {
                [self checkCurrentKey:completionBlock platform:platform attempt:attempt + 1];
                return nil;
            }

            // If the task completed, then check if the apiKey is valid.
            self.currentKeyToCheck = nil;
            BOOL valid = NO;
            if (task.result) {
                VROApiKey *key = task.result;
                // if the key is valid, then we should increment its corresponding counter in the metrics table.
                if ([key.Valid isEqualToString:@"true"]) {
                    valid = YES;
                    VROApiKeyMetrics *metricsRecord = [[VROApiKeyMetrics alloc] initWithApiKey:key.ApiKey platform:platform];
                    VROApiMetricsIncrementRequest *incrementRequest =
                            [[VROApiMetricsIncrementRequest alloc] initWithMetrics:metricsRecord
                                                                   dynamoObjMapper:self.dynamoObjectMapper];
                    [incrementRequest run];
                }
            }
            completionBlock(valid);
            return nil;
        }
    };
}

@end
