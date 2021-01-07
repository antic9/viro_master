//
//  VROObjectRecognizeriOS.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 1/10/19.
//  Copyright © 2019 Viro Media. All rights reserved.
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

#include "VROObjectRecognizeriOS.h"
#include "VROLog.h"
#include "VROTime.h"
#include "VROARFrameiOS.h"
#import "VRODriverOpenGLiOS.h"

VROObjectRecognizeriOS::VROObjectRecognizeriOS() {
    _currentImage = nil;
    _visionQueue = dispatch_queue_create("com.viro.serialVisionQueue", DISPATCH_QUEUE_SERIAL);
}

bool VROObjectRecognizeriOS::initObjectTracking(VROCameraPosition position,
                                                std::shared_ptr<VRODriver> driver) {
    
    
    _model = nullptr; // Disabled, model removed [[[model_yolo_coco alloc] init] model];
    _coreMLModel =  [VNCoreMLModel modelForMLModel:_model error:nil];
    _visionRequest = [[VNCoreMLRequest alloc] initWithModel:_coreMLModel
                                          completionHandler:(VNRequestCompletionHandler)^(VNRequest *request, NSError *error) {
                                              processVisionResults(request, error);
                                          }];
    _visionRequest.imageCropAndScaleOption = VNImageCropAndScaleOptionScaleFill;
    
    return true;
}

void VROObjectRecognizeriOS::startObjectTracking() {
    
}

void VROObjectRecognizeriOS::stopObjectTracking() {
    
}

void VROObjectRecognizeriOS::update(const VROARFrame *frame) {
    const VROARFrameiOS *frameiOS = dynamic_cast<const VROARFrameiOS *>(frame);

    CVPixelBufferRef cameraImage = frameiOS->getImage();
    VROMatrix4f transform = frameiOS->getViewportToCameraImageTransform().invert();
    VROCameraOrientation orientation = frameiOS->getOrientation();

    // Only process one image at a time
    if (_currentImage != nil) {
        return;
    }
    _currentImage = cameraImage;
    
    dispatch_async(_visionQueue, ^{
        trackCurrentImage(transform, orientation);
    });
}

// Invoked on the _visionQueue
void VROObjectRecognizeriOS::trackCurrentImage(VROMatrix4f transform, VROCameraOrientation orientation) {
    NSDictionary *visionOptions = [NSDictionary dictionary];
    
    // The logic below derives the _transform matrix, which is used to convert *rotated* image
    // coordinates to viewport coordinates. This matrix is derived from the scale and translation
    // components of ARKit's displayTransform metrix (we remove the rotation part from the ARKit
    // matrix because iOS will automatically rotate the image before inputting it into the CoreML
    // model).
    VROVector3f scale = transform.extractScale();
    VROVector3f translation = transform.extractTranslation();
    
    if (orientation == VROCameraOrientation::Portrait || orientation == VROCameraOrientation::PortraitUpsideDown) {
        // Remove rotation from the transformation matrix. Since this was a 90 degree rotation, X and Y are
        // reversed.
        _transform[0] = scale.y;
        _transform[1] = 0;
        _transform[4] = 0;
        _transform[5] = scale.x;
        _transform[12] = (1 - scale.y) / 2.0;
        _transform[13] = translation.y;
        
        // iOS always rotates the image right-side up before inputting into a CoreML model for
        // a vision request. We ensure it rotates correctly by specifying the orientation of the
        // image with respect to the device.
        CGImagePropertyOrientation orientation = kCGImagePropertyOrientationRight;
        
        // By wrapping the CVPixelBuffer in a CIImage, iOS will automatically convert from
        // YCbCr to RGB (note: this is undocumented, but works).
        CIImage *ciImage = [[CIImage alloc] initWithCVPixelBuffer:_currentImage];
        VNImageRequestHandler *handler = [[VNImageRequestHandler alloc] initWithCIImage:ciImage
                                                                            orientation:orientation
                                                                                options:visionOptions];
        [handler performRequests:@[_visionRequest] error:nil];
    }
    else if (orientation == VROCameraOrientation::LandscapeLeft) {
        // Remove rotation from the transformation matrix
        _transform[0] = scale.x;
        _transform[1] = 0;
        _transform[4] = 0;
        _transform[5] = scale.y;
        _transform[12] = (1 - scale.x) / 2.0;
        _transform[13] = (1 - scale.y) / 2.0;
        
        CGImagePropertyOrientation orientation = kCGImagePropertyOrientationDown;
        CIImage *ciImage = [[CIImage alloc] initWithCVPixelBuffer:_currentImage];
        VNImageRequestHandler *handler = [[VNImageRequestHandler alloc] initWithCIImage:ciImage
                                                                            orientation:orientation
                                                                                options:visionOptions];
        [handler performRequests:@[_visionRequest] error:nil];
    }
    else if (orientation == VROCameraOrientation::LandscapeRight) {
        // In landscape right, the camera image is already right-side up, and ready for the ML
        // algorithm.
        _transform = transform;
        
        CIImage *ciImage = [[CIImage alloc] initWithCVPixelBuffer:_currentImage];
        VNImageRequestHandler *handler = [[VNImageRequestHandler alloc] initWithCIImage:ciImage options:visionOptions];
        [handler performRequests:@[_visionRequest] error:nil];
    }
}

// Invoked on the _visionQueue
void VROObjectRecognizeriOS::processVisionResults(VNRequest *request, NSError *error) {
    NSArray *array = [request results];
    NSLog(@"Number of results %d", (int) array.count);
    
    std::map<std::string, std::vector<VRORecognizedObject>> objects;
    
    for (VNRecognizedObjectObservation *observation in array) {
        CGRect bounds = observation.boundingBox;
        
        VROBoundingBox box(bounds.origin.x,
                           (bounds.origin.x + bounds.size.width),
                           (1.0 - bounds.origin.y),
                           (1.0 - bounds.origin.y - bounds.size.height),
                           0, 0);
        box = box.transform(_transform);
        
        for (VNClassificationObservation *classification in observation.labels) {
            if (classification.confidence > 0.8) {
                std::string className = std::string([classification.identifier UTF8String]);
                
                NSLog(@"   Label %@ confidence %f", classification.identifier, classification.confidence);
                objects[className].push_back({ className, box, classification.confidence });
            }
        }
    }
    
    dispatch_async(dispatch_get_main_queue(), ^{
        std::shared_ptr<VROObjectRecognizerDelegate> delegate = _objectRecognizerDelegate_w.lock();
        if (delegate) {
            delegate->onObjectsFound(objects);
        }
    });
    _currentImage = nil;
}
