//
//  Camera_JNI.h
//  ViroRenderer
//
//  Copyright © 2018 Viro Media. All rights reserved.
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

#ifndef ANDROID_CAMERA_JNI_H
#define ANDROID_CAMERA_JNI_H

#include <memory>
#include "VRONodeCamera.h"
#include "VROCamera.h"
#include "VROPlatformUtil.h"
#include "VROTime.h"
#include "ViroUtils_JNI.h"

#include "VRODefines.h"
#include VRO_C_INCLUDE

class CameraDelegateJNI : public VROCameraDelegate {
public:
    CameraDelegateJNI(VRO_OBJECT obj) :
        _javaObject(VRO_OBJECT_NULL) {

        VRO_ENV env = VROPlatformGetJNIEnv();
        _javaObject = VRO_NEW_WEAK_GLOBAL_REF(obj);
    }

    virtual ~CameraDelegateJNI() {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_DELETE_WEAK_GLOBAL_REF(_javaObject);
    }


    /*
     Called from VRORenderer to notify the JNI bridge with a camera transformation update.
     Filtering is also performed here to reduce the number of bridge synchronization calls.
     */
    void onCameraTransformationUpdate(VROVector3f pos, VROVector3f rot, VROVector3f forward) {
        if (!shouldUpdate(pos, forward)) {
            return;
        }

        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_WEAK jObjWeak = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);
        VROPlatformDispatchAsyncApplication([jObjWeak, pos, rot, forward] {
            VRO_ENV env = VROPlatformGetJNIEnv();
            VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(jObjWeak);
            if (VRO_IS_OBJECT_NULL(localObj)) {
                return;
            }
            VRO_FLOAT_ARRAY jPos = ARUtilsCreateFloatArrayFromVector3f(pos);
            VRO_FLOAT_ARRAY jRot = ARUtilsCreateFloatArrayFromVector3f(rot);
            VRO_FLOAT_ARRAY jForward = ARUtilsCreateFloatArrayFromVector3f(forward);
            VROPlatformCallHostFunction(localObj, "onCameraTransformationUpdate", "([F[F[F)V",
                                        jPos, jRot, jForward);
            VRO_DELETE_LOCAL_REF(localObj);
            VRO_DELETE_WEAK_GLOBAL_REF(jObjWeak);
        });
    }

    /*
     Returns true if the camera has moved sufficiently beyond a certain distance or rotation
     threshold that warrants a transformation update across the JNI bridge.
     */
    bool shouldUpdate(VROVector3f pos, VROVector3f forward) {
        double currentRenderTime = VROTimeCurrentMillis();
        if (_lastSampleTimeMs + 16 >= currentRenderTime) {
            return false;
        }

        // Determine if we need to flush an update with stale camera data.
        bool shouldBypassFilters = shouldForceStaleUpdate(pos, forward);

        // Only trigger delegates if the camera has moved / rotated a sufficient amount
        // (and if there is no stale transforms to be flushed).
        if (!shouldBypassFilters && _lastPositionUpdate.distance(pos) < _distanceThreshold &&
            _lastForwardVectorUpdate.angleWithVector(forward) < _angleThreshold) {
            return false;
        }

        _lastSampleTimeMs = currentRenderTime;
        _lastForwardVectorUpdate = forward;
        _lastPositionUpdate = pos;
        return true;
    }

    /*
     Returns true to flush a stale camera transform update. This occurs if a new Camera
     transformation has been receieved, but is not significant enough to satisfy the
     distance/rotation thresholds for a stale period length of time.
     */
    bool shouldForceStaleUpdate(VROVector3f pos, VROVector3f forward) {
        // If different, refresh stale counter, return false signaling that we do not need
        // to force a stale update.
        if (_lastSampledPos != pos || _lastSampledForward != forward) {
            _sampledStaleCount = 0;
            _lastSampledPos = pos;
            _lastSampledForward = forward;
            return false;
        }

        // Else If the position and forward remain the same, perform stale checks.
        // Here, we return true if the data is stale with a lifespan of _stalePeriodThreshold.
        if ( _sampledStaleCount < _stalePeriodThreshold) {
            _sampledStaleCount++;
            return false;
        } else if (_sampledStaleCount == _stalePeriodThreshold) {
            _sampledStaleCount++;
        } else if (_sampledStaleCount > _stalePeriodThreshold) {
            return false;
        }

        _lastSampledPos = pos;
        _lastSampledForward = forward;
        return true;
    }

private:
    VRO_OBJECT _javaObject;

    /*
     Last time stamp at which we have been notified of transformation updates.
     */
    double _lastSampleTimeMs = 0;
    VROVector3f _lastSampledPos;
    VROVector3f _lastSampledForward;

    /*
     Distance threshold filters to prevent thrashing the UI thread with updates.
     */
    const double _distanceThreshold = 0.01;
    VROVector3f _lastPositionUpdate;

    /*
     Rotation threshold filters to prevent thrashing the UI thread with updates.
     */
    const double _angleThreshold = 0.017;
    VROVector3f _lastForwardVectorUpdate;

    /*
     Period threshold after which the last known transform is considered stale and thus
     an update flush is required.
     */
    const int _stalePeriodThreshold = 20;
    int _sampledStaleCount = 0;
};

#endif //ANDROID_CAMERA_JNI_H
