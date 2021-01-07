//
//  EventDelegate_JNI.cpp
//  ViroRenderer
//
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

#include "EventDelegate_JNI.h"
#include "Node_JNI.h"
#include "ViroUtils_JNI.h"
#include "VROARPointCloud.h"

#if VRO_PLATFORM_ANDROID
#include "arcore/ARUtils_JNI.h"

#define VRO_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_com_viro_core_EventDelegate_##method_name
#else
#define VRO_METHOD(return_type, method_name) \
    return_type EventDelegate_##method_name
#endif

extern "C" {

VRO_METHOD(VRO_REF(EventDelegate_JNI), nativeCreateDelegate)(VRO_NO_ARGS) {
   VRO_METHOD_PREAMBLE;
   std::shared_ptr<EventDelegate_JNI> delegate = std::make_shared<EventDelegate_JNI>(obj, env);
   return VRO_REF_NEW(EventDelegate_JNI, delegate);
}

VRO_METHOD(void, nativeDestroyDelegate)(VRO_ARGS
                                        VRO_REF(EventDelegate_JNI) native_node_ref) {
    // TODO: figure out why this is needed
    VROPlatformDispatchAsyncRenderer([native_node_ref]{
        VRO_REF_DELETE(VRONode, native_node_ref);
    });
}

VRO_METHOD(void, nativeEnableEvent)(VRO_ARGS
                                    VRO_REF(EventDelegate_JNI) native_node_ref,
                                    VRO_INT eventTypeId,
                                    VRO_BOOL enabled) {

    std::weak_ptr<EventDelegate_JNI> delegate_w = VRO_REF_GET(EventDelegate_JNI, native_node_ref);
    VROPlatformDispatchAsyncRenderer([delegate_w, eventTypeId, enabled] {
        std::shared_ptr<EventDelegate_JNI> delegate = delegate_w.lock();
        if (!delegate) {
            return;
        }
        VROEventDelegate::EventAction eventType = static_cast<VROEventDelegate::EventAction>(eventTypeId);
        delegate->setEnabledEvent(eventType, enabled);
    });
}

VRO_METHOD(void, nativeSetTimeToFuse)(VRO_ARGS
                                      VRO_REF(EventDelegate_JNI) native_node_ref,
                                      VRO_FLOAT durationInMillis) {
    std::weak_ptr<EventDelegate_JNI> delegate_w = VRO_REF_GET(EventDelegate_JNI, native_node_ref);
    VROPlatformDispatchAsyncRenderer([delegate_w, durationInMillis] {
        std::shared_ptr<EventDelegate_JNI> delegate = delegate_w.lock();
        if (!delegate) {
            return;
        }
        delegate->setTimeToFuse(durationInMillis);
    });
}

}  // extern "C"

/*
 This integer represents a value which no Node should return when getUniqueID()
 is invoked. This value is derived from the behavior of sUniqueIDGenerator in
 VRONode.h.
 */
static int sNullNodeID = -1;

void EventDelegate_JNI::onHover(int source, std::shared_ptr<VRONode> node, bool isHovering, std::vector<float> position) {
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, source, node, isHovering, position] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        VRO_FLOAT_ARRAY positionArray;
        if (position.size() == 3) {
            int returnLength = 3;
            positionArray = VRO_NEW_FLOAT_ARRAY(returnLength);

            VRO_FLOAT tempArr[returnLength];
            tempArr[0] = position.at(0);
            tempArr[1] = position.at(1);
            tempArr[2] = position.at(2);
            VRO_FLOAT_ARRAY_SET(positionArray, 0, 3, tempArr);
        } else {
            positionArray = nullptr;
        }

        int nodeId = node != nullptr ? node->getUniqueID() : sNullNodeID;
        VROPlatformCallHostFunction(localObj,
                                    "onHover", "(IIZ[F)V", source, nodeId, isHovering, positionArray);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onClick(int source, std::shared_ptr<VRONode> node, ClickState clickState, std::vector<float> position) {
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, source, node, clickState, position] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        VRO_FLOAT_ARRAY positionArray;
        if (position.size() == 3) {
            int returnLength = 3;
            positionArray = VRO_NEW_FLOAT_ARRAY(returnLength);
            VRO_FLOAT tempArr[returnLength];
            tempArr[0] = position.at(0);
            tempArr[1] = position.at(1);
            tempArr[2] = position.at(2);
            VRO_FLOAT_ARRAY_SET(positionArray, 0, 3, tempArr);
        } else {
            positionArray = nullptr;
        }

        int nodeId = node != nullptr ? node->getUniqueID() : sNullNodeID;
        VROPlatformCallHostFunction(localObj,
                                    "onClick", "(III[F)V", source, nodeId, clickState, positionArray);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onTouch(int source, std::shared_ptr<VRONode> node, TouchState touchState, float x, float y){
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, source, node, touchState, x, y] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        int nodeId = node != nullptr ? node->getUniqueID() : sNullNodeID;
        VROPlatformCallHostFunction(localObj,
                                    "onTouch", "(IIIFF)V", source, nodeId, touchState, x, y);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onMove(int source, std::shared_ptr<VRONode> node, VROVector3f rotation, VROVector3f position, VROVector3f forwardVec) {
    //No-op
}

void EventDelegate_JNI::onControllerStatus(int source, ControllerStatus status) {
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, source, status] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        VROPlatformCallHostFunction(localObj,
                                    "onControllerStatus", "(II)V", source, status);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onGazeHit(int source, std::shared_ptr<VRONode> node, float distance, VROVector3f hitLocation) {
    //No-op
}

void EventDelegate_JNI::onSwipe(int source, std::shared_ptr<VRONode> node, SwipeState swipeState) {
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, source, node, swipeState] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        int nodeId = node != nullptr ? node->getUniqueID() : sNullNodeID;
        VROPlatformCallHostFunction(localObj,
                                    "onSwipe", "(III)V", source, nodeId, swipeState);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onScroll(int source, std::shared_ptr<VRONode> node, float x, float y) {
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, source, node, x, y] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        int nodeId = node != nullptr ? node->getUniqueID() : sNullNodeID;
        VROPlatformCallHostFunction(localObj,
                                    "onScroll", "(IIFF)V", source, nodeId, x, y);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onDrag(int source, std::shared_ptr<VRONode> node, VROVector3f newPosition) {
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, source, node, newPosition] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        int nodeId = node != nullptr ? node->getUniqueID() : sNullNodeID;
        VROPlatformCallHostFunction(localObj,
                                    "onDrag", "(IIFFF)V", source, nodeId, newPosition.x, newPosition.y,
                                    newPosition.z);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onFuse(int source, std::shared_ptr<VRONode> node, float timeToFuseRatio){
    /**
     * As onFuse is also used by internal components to update ui based
     * on timeToFuse ratio, we only want to notify our bridge components
     * if we have successfully fused (if timeToFuseRatio has counted down to 0).
     */
    if (timeToFuseRatio > 0.0f){
        return;
    }

    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, source, node, timeToFuseRatio] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        int nodeId = node != nullptr ? node->getUniqueID() : sNullNodeID;
        VROPlatformCallHostFunction(localObj,
                                    "onFuse", "(II)V", source, nodeId, timeToFuseRatio);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onPinch(int source, std::shared_ptr<VRONode> node, float scaleFactor, PinchState pinchState) {
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, source, node, scaleFactor, pinchState] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        int nodeId = node != nullptr ? node->getUniqueID() : sNullNodeID;
        VROPlatformCallHostFunction(localObj,
                                    "onPinch", "(IIFI)V", source, nodeId, scaleFactor, pinchState);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onRotate(int source, std::shared_ptr<VRONode> node, float rotationRadians, RotateState rotateState) {
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, source, node, rotationRadians, rotateState] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        int nodeId = node != nullptr ? node->getUniqueID() : sNullNodeID;
        VROPlatformCallHostFunction(localObj,
                                    "onRotate", "(IIFI)V", source, nodeId, rotationRadians, rotateState);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onCameraARHitTest(std::vector<std::shared_ptr<VROARHitTestResult>> results) {
#if VRO_PLATFORM_ANDROID
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, results] {
        VRO_ENV env = VROPlatformGetJNIEnv();

        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }
        //start code to convert VROARHitTestResult to ARHitTestResult
        VRO_OBJECT_ARRAY resultsArray = VRO_NEW_OBJECT_ARRAY(results.size(), "com/viro/core/ARHitTestResult");
        for (int i = 0; i < results.size(); i++) {
            VRO_OBJECT jresult = ARUtilsCreateARHitTestResult(results[i]);
            VRO_OBJECT_ARRAY_SET(resultsArray, i, jresult);
        }

        VROPlatformCallHostFunction(localObj,
                                    "onCameraARHitTest", "([Lcom/viro/core/ARHitTestResult;)V", resultsArray);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
#endif
}

void EventDelegate_JNI::onARPointCloudUpdate(std::shared_ptr<VROARPointCloud> pointCloud) {
#if VRO_PLATFORM_ANDROID
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    // So it turns out that ARCore returns us garbage values (NaN) when the camera is obscured
    // so, rather than waste all the time going up to Java, check 1 value right here and
    // return if it is a NaN.
    if (isnan(pointCloud->getPoints()[0].x)) {
        return;
    }

    VROPlatformDispatchAsyncApplication([weakObj, pointCloud] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        VRO_OBJECT jPointCloud = ARUtilsCreateARPointCloud(pointCloud);
        VROPlatformCallHostFunction(localObj, "onARPointCloudUpdate", "(Lcom/viro/core/ARPointCloud;)V", jPointCloud);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
#endif
}


void EventDelegate_JNI::onCameraTransformUpdate(VROVector3f position, VROVector3f rotation, VROVector3f forward, VROVector3f up) {
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, position, rotation, forward, up] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        VROPlatformCallHostFunction(localObj, "onCameraTransformUpdate", "(FFFFFFFFFFFF)V",
                                    position.x, position.y, position.z, rotation.x, rotation.y, rotation.z,
                                    forward.x, forward.y, forward.z, up.x, up.y, up.z);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}