//
//  VROScreen.c
//  ViroRenderer
//
//  Created by Raj Advani on 10/23/15.
//  Copyright © 2015 Viro Media. All rights reserved.
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

#include "VROScreen.h"
#if VRO_METAL

#include <sys/utsname.h>

// Enable to make the lens-distorted viewports slightly
// smaller on iPhone 6/6+ and bigger on iPhone 5/5s
#define SCREEN_PARAMS_CORRECT_IPHONE_VIEWPORTS 1

#define CBScreenIsRetina() ([[UIScreen mainScreen] scale] == 2.0)
#define CBScreenIsIpad() (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
#define CBScreenIsIphone() (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
#define CBScreenIsIphone4Width() (CBScreenIsIphone() && [UIScreen mainScreen].sizeFixedToPortrait.width == 320.0)
#define CBScreenIsIphone4Height() (CBScreenIsIphone() && [UIScreen mainScreen].sizeFixedToPortrait.height == 480.0)
#define CBScreenIsIphone5Width() (CBScreenIsIphone4Width())
#define CBScreenIsIphone5Height() (CBScreenIsIphone() && [UIScreen mainScreen].sizeFixedToPortrait.height == 568.0)
#define CBScreenIsIphone6Width() (CBScreenIsIphone() && [UIScreen mainScreen].sizeFixedToPortrait.width == 375.0)
#define CBScreenIsIphone6Height() (CBScreenIsIphone() && [UIScreen mainScreen].sizeFixedToPortrait.height == 667.0)
#define CBScreenIsIphone6PlusWidth() (CBScreenIsIphone() && [[UIScreen mainScreen] scale] == 3.0f && [UIScreen mainScreen].sizeFixedToPortrait.width == 414.0)
#define CBScreenIsIphone6PlusHeight() (CBScreenIsIphone() && [[UIScreen mainScreen] scale] == 3.0f && [UIScreen mainScreen].sizeFixedToPortrait.height == 736.0)

@interface UIScreen (VROOrientationAware)

- (CGSize)orientationAwareSize;
- (CGSize)sizeFixedToPortrait;

@end

@implementation UIScreen (VROOrientationAware)

- (CGSize)orientationAwareSize {
    return self.bounds.size;
}

- (CGSize)sizeFixedToPortrait {
    CGSize size = self.bounds.size;
    return CGSizeMake(MIN(size.width, size.height), MAX(size.width, size.height));
}

@end

VROScreen::VROScreen(UIScreen *screen) {
    _screen = screen;
    if ([screen respondsToSelector:@selector(nativeScale)]) {
        _scale = screen.nativeScale;
    }
    else {
        _scale = screen.scale;
    }
    
    float screenPixelsPerInch = pixelsPerInch(screen);
    
    const float metersPerInch = 0.0254f;
    const float defaultBorderSizeMeters = 0.003f;
    _xMetersPerPixel = (metersPerInch / screenPixelsPerInch);
    _yMetersPerPixel = (metersPerInch / screenPixelsPerInch);
    
    _borderSizeMeters = defaultBorderSizeMeters;
    
#if SCREEN_PARAMS_CORRECT_IPHONE_VIEWPORTS
    if (CBScreenIsIphone5Width()) {
        _borderSizeMeters = 0.006f;
    }
    else if (CBScreenIsIphone6Width() || CBScreenIsIphone6PlusWidth()) {
        _borderSizeMeters = 0.001f;
    }
#endif
}

int VROScreen::getWidth() const {
    return [_screen orientationAwareSize].width * _scale;
}

int VROScreen::getHeight() const {
    return [_screen orientationAwareSize].height * _scale;
}

float VROScreen::getWidthInMeters() const {
    return getWidth() * _xMetersPerPixel;
}

float VROScreen::getHeightInMeters() const {
    return getHeight() * _yMetersPerPixel;
}

void VROScreen::setBorderSizeInMeters(float screenBorderSize) {
    _borderSizeMeters = screenBorderSize;
}

float VROScreen::getBorderSizeInMeters() const {
    return _borderSizeMeters;
}

float VROScreen::pixelsPerInch(UIScreen *screen) {
    // Default iPhone retina pixels per inch
    float pixelsPerInch = 163.0f * _scale;
    struct utsname sysinfo;
    
    if (uname(&sysinfo) == 0) {
        NSString *identifier = [NSString stringWithUTF8String:sysinfo.machine];
        NSArray *deviceClassArray =
        @[
          // iPads
          @{@"identifiers":
          @[@"iPad1,1",
            @"iPad2,1", @"iPad2,2", @"iPad2,3", @"iPad2,4",
            @"iPad3,1", @"iPad3,2", @"iPad3,3", @"iPad3,4",
            @"iPad3,5", @"iPad3,6", @"iPad4,1", @"iPad4,2"],
          @"pointsPerInch": @132.0f},
          // iPhones, iPad Minis and simulators
          @{@"identifiers":
          @[@"iPod5,1",
            @"iPhone1,1", @"iPhone1,2",
            @"iPhone2,1",
            @"iPhone3,1", @"iPhone3,2", @"iPhone3,3",
            @"iPhone4,1",
            @"iPhone5,1", @"iPhone5,2", @"iPhone5,3", @"iPhone5,4",
            @"iPhone6,1", @"iPhone6,2",
            @"iPhone7,1", @"iPhone7,2",
            @"iPhone8,1", @"iPhone8,2",
            @"iPad2,5", @"iPad2,6", @"iPad2,7",
            @"iPad4,4", @"iPad4,5",
            @"i386", @"x86_64"],
          @"pointsPerInch":  @163.0f } ];
        
        for (NSDictionary *deviceClass in deviceClassArray) {
            for (NSString *deviceId in deviceClass[@"identifiers"]) {
                if ([identifier isEqualToString:deviceId]) {
                    pixelsPerInch = [deviceClass[@"pointsPerInch"] floatValue] * _scale;
                    break;
                }
            }
        }
    }
    
    return pixelsPerInch;
}

#endif
