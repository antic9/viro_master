//
//  VROHeadTracker.h
//  ViroRenderer
//
//  Created by Raj Advani on 10/26/15.
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

#ifndef __CardboardSDK_iOS__HeadTracker__
#define __CardboardSDK_iOS__HeadTracker__

#include "OrientationEKF.h"

#import <CoreMotion/CoreMotion.h>
#import <GLKit/GLKit.h>
#import "VROMatrix4f.h"

/*
 Continually tracks sensor data and returns the head rotation matrix.
 This head rotation matrix can be used to compute the view matrix for
 the renderer.
 */
class VROHeadTracker {
    
public:
    
    VROHeadTracker();
    virtual ~VROHeadTracker();
    
    void startTracking(UIInterfaceOrientation orientation);
    void stopTracking();
    VROMatrix4f getHeadRotation();
    
    void updateDeviceOrientation(UIInterfaceOrientation orientation);
    bool isReady();
    
private:
    
    CMMotionManager *_motionManager;
    size_t _sampleCount;
    OrientationEKF *_tracker;
    
    GLKMatrix4 _worldToDeviceMatrix;
    GLKMatrix4 _IRFToWorldMatrix;
    GLKMatrix4 _correctedIRFToWorldMatrix;
    
    VROMatrix4f _lastHeadRotation;
    NSTimeInterval _lastGyroEventTimestamp;
    
    bool _headingCorrectionComputed;
    
};

#endif
