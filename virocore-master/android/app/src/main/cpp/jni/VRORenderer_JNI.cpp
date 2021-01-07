//
//  VRORenderer_JNI.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 11/8/16.
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

#include <jni.h>
#include <memory>
#include <PersistentRef.h>
#include <VROARHitTestResult.h>
#include <VROFrameListener.h>
#include "vr/gvr/capi/include/gvr.h"
#include "vr/gvr/capi/include/gvr_audio.h"
#include "VROProjector.h"
#include "VROSceneRendererGVR.h"
#include "VROSceneRendererOVR.h"
#include "VROSceneRendererSceneView.h"
#include "VROPlatformUtil.h"
#include "VROSample.h"
#include "Node_JNI.h"
#include "VROSceneController.h"
#include "VRORenderer_JNI.h"
#include "VROReticle.h"
#include "SceneController_JNI.h"
#include "FrameListener_JNI.h"
#include "ViroUtils_JNI.h"
#include "Camera_JNI.h"
#include "VRORenderer.h"
#include "VROChoreographer.h"
#include "ViroUtils_JNI.h"

#if VRO_PLATFORM_ANDROID
#define VRO_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_com_viro_core_Renderer_##method_name
#endif

extern "C" {

// The renderer test runs VROSample.cpp, for fast prototyping when working
// on renderer features (no bridge integration).
static bool kRunRendererTest = false;
static std::shared_ptr<VROSample> sample;

VRO_METHOD(jlong, nativeCreateRendererGVR)(VRO_ARGS
                                           jobject class_loader,
                                           jobject android_context,
                                           jobject asset_mgr,
                                           jobject platform_util,
                                           jlong native_gvr_api,
                                           jboolean enableShadows,
                                           jboolean enableHDR,
                                           jboolean enablePBR,
                                           jboolean enableBloom) {
    VROPlatformSetType(VROPlatformType::AndroidGVR);

    std::shared_ptr<gvr::AudioApi> gvrAudio = std::make_shared<gvr::AudioApi>();
    gvrAudio->Init(env, android_context, class_loader, GVR_AUDIO_RENDERING_BINAURAL_HIGH_QUALITY);
    VROPlatformSetEnv(env, android_context, asset_mgr, platform_util);

    VRORendererConfiguration config;
    config.enableShadows = enableShadows;
    config.enableHDR = enableHDR;
    config.enablePBR = enablePBR;
    config.enableBloom = enableBloom;

    gvr_context *gvrContext = reinterpret_cast<gvr_context *>(native_gvr_api);
    std::shared_ptr<VROSceneRenderer> renderer
            = std::make_shared<VROSceneRendererGVR>(config, gvrContext, gvrAudio);
    return Renderer::jptr(renderer);
}

VRO_METHOD(jlong, nativeCreateRendererOVR)(VRO_ARGS
                                           jobject class_loader,
                                           jobject android_context,
                                           jobject view,
                                           jobject activity,
                                           jobject asset_mgr,
                                           jobject platform_util,
                                           jboolean enableShadows,
                                           jboolean enableHDR,
                                           jboolean enablePBR,
                                           jboolean enableBloom) {
    VROPlatformSetType(VROPlatformType::AndroidOVR);

    std::shared_ptr<gvr::AudioApi> gvrAudio = std::make_shared<gvr::AudioApi>();
    gvrAudio->Init(env, android_context, class_loader, GVR_AUDIO_RENDERING_BINAURAL_HIGH_QUALITY);
    VROPlatformSetEnv(env, android_context, asset_mgr, platform_util);

    VRORendererConfiguration config;
    config.enableShadows = enableShadows;
    config.enableHDR = enableHDR;
    config.enablePBR = enablePBR;
    config.enableBloom = enableBloom;

    std::shared_ptr<VROSceneRenderer> renderer
            = std::make_shared<VROSceneRendererOVR>(config, gvrAudio, view, activity, env);
    return Renderer::jptr(renderer);
}

VRO_METHOD(jlong, nativeCreateRendererSceneView)(VRO_ARGS
                                                 jobject class_loader,
                                                 jobject android_context,
                                                 jobject view,
                                                 jobject asset_mgr,
                                                 jobject platform_util,
                                                 jboolean enableShadows,
                                                 jboolean enableHDR,
                                                 jboolean enablePBR,
                                                 jboolean enableBloom) {
    VROPlatformSetType(VROPlatformType::AndroidSceneView);

    std::shared_ptr<gvr::AudioApi> gvrAudio = std::make_shared<gvr::AudioApi>();
    gvrAudio->Init(env, android_context, class_loader, GVR_AUDIO_RENDERING_BINAURAL_HIGH_QUALITY);
    VROPlatformSetEnv(env, android_context, asset_mgr, platform_util);

    VRORendererConfiguration config;
    config.enableShadows = enableShadows;
    config.enableHDR = enableHDR;
    config.enablePBR = enablePBR;
    config.enableBloom = enableBloom;

    std::shared_ptr<VROSceneRenderer> renderer
            = std::make_shared<VROSceneRendererSceneView>(config, gvrAudio, view);
    return Renderer::jptr(renderer);
}

VRO_METHOD(void, nativeDestroyRenderer)(VRO_ARGS
                                        jlong native_renderer) {
    Renderer::native(native_renderer)->onDestroy();
    VROThreadRestricted::unsetThread();

    delete reinterpret_cast<PersistentRef<VROSceneRenderer> *>(native_renderer);

    // Once the renderer dies, release/reset VROPlatformUtils stuff
    VROPlatformReleaseEnv();
}

VRO_METHOD(void, nativeInitializeGL)(VRO_ARGS
                                     jlong native_renderer,
                                     jboolean sRGBFramebuffer,
                                     jboolean testingMode) {

    VROThreadRestricted::setThread(VROThreadName::Renderer);
    std::shared_ptr<VROSceneRenderer> sceneRenderer = Renderer::native(native_renderer);

    std::shared_ptr<VRODriverOpenGLAndroid> driver = std::dynamic_pointer_cast<VRODriverOpenGLAndroid>(sceneRenderer->getDriver());
    driver->setSRGBFramebuffer(sRGBFramebuffer);

    kRunRendererTest = testingMode;
    if (kRunRendererTest) {
        sample = std::make_shared<VROSample>();
        sceneRenderer->setRenderDelegate(sample);

        sample->loadTestHarness(sceneRenderer->getRenderer(), sceneRenderer->getFrameSynchronizer(),
                                sceneRenderer->getDriver());
        sceneRenderer->setSceneController(sample->getSceneController());
        if (sample->getPointOfView()) {
            sceneRenderer->getRenderer()->setPointOfView(sample->getPointOfView());
        }
    }
    sceneRenderer->initGL();
}

VRO_METHOD(void, nativeDrawFrame)(VRO_ARGS
                                  jlong native_renderer) {
    Renderer::native(native_renderer)->onDrawFrame();
}

VRO_METHOD (void, nativeOnKeyEvent)(VRO_ARGS
                                    jlong native_renderer,
                                    int keyCode,
                                    int action ){
    std::weak_ptr<VROSceneRenderer> renderer_w = Renderer::native(native_renderer);
    VROPlatformDispatchAsyncRenderer([renderer_w, keyCode, action] {
        std::shared_ptr<VROSceneRenderer> renderer = renderer_w.lock();
        if (!renderer) {
            return;
        }
        renderer->onKeyEvent(keyCode, action);
    });
}

VRO_METHOD(void, nativeOnTouchEvent)(VRO_ARGS
                                     jlong native_renderer,
                                     VRO_INT onTouchAction,
                                     float xPos,
                                     float yPos) {
    std::weak_ptr<VROSceneRenderer> renderer_w = Renderer::native(native_renderer);
    VROPlatformDispatchAsyncRenderer([renderer_w, onTouchAction, xPos, yPos] {
        std::shared_ptr<VROSceneRenderer> renderer = renderer_w.lock();
        if (!renderer) {
            return;
        }
        renderer->onTouchEvent(onTouchAction, xPos, yPos);
    });
}

VRO_METHOD(void, nativeOnPinchEvent) (VRO_ARGS
                                      jlong native_renderer,
                                      VRO_INT pinchState,
                                      VRO_FLOAT scaleFactor,
                                      VRO_FLOAT viewportX,
                                      VRO_FLOAT viewportY) {
    std::weak_ptr<VROSceneRenderer> renderer_w = Renderer::native(native_renderer);
    VROPlatformDispatchAsyncRenderer([renderer_w, pinchState, scaleFactor, viewportX, viewportY] {
        std::shared_ptr<VROSceneRenderer> renderer = renderer_w.lock();
        if (!renderer) {
            return;
        }
        renderer->onPinchEvent(pinchState, scaleFactor, viewportX, viewportY);
    });
}

VRO_METHOD(void, nativeOnRotateEvent) (VRO_ARGS
                                       jlong native_renderer,
                                       VRO_INT rotateState,
                                       VRO_FLOAT rotateRadians,
                                       VRO_FLOAT viewportX,
                                       VRO_FLOAT viewportY) {
    std::weak_ptr<VROSceneRenderer> renderer_w = Renderer::native(native_renderer);
    VROPlatformDispatchAsyncRenderer([renderer_w, rotateState, rotateRadians, viewportX, viewportY] {
        std::shared_ptr<VROSceneRenderer> renderer = renderer_w.lock();
        if (!renderer) {
            return;
        }
        renderer->onRotateEvent(rotateState, rotateRadians, viewportX, viewportY);
    });
}

VRO_METHOD(void, nativeSetVRModeEnabled)(VRO_ARGS
                                         jlong nativeRenderer, jboolean enabled) {
    std::weak_ptr<VROSceneRenderer> renderer_w = Renderer::native(nativeRenderer);
    VROPlatformDispatchAsyncRenderer([renderer_w, enabled] {
        std::shared_ptr<VROSceneRenderer> renderer = renderer_w.lock();
        if (!renderer) {
            return;
        }
        renderer->setVRModeEnabled(enabled);
    });
}

VRO_METHOD(void, nativeOnStart)(VRO_ARGS
                                jlong native_renderer) {
        Renderer::native(native_renderer)->onStart();
}

VRO_METHOD(void, nativeOnPause)(VRO_ARGS
                                jlong native_renderer) {
        Renderer::native(native_renderer)->onPause();
}

VRO_METHOD(void, nativeOnResume)(VRO_ARGS
                                 jlong native_renderer) {
        Renderer::native(native_renderer)->onResume();
}

VRO_METHOD(void, nativeOnStop)(VRO_ARGS
                               jlong native_renderer) {
        Renderer::native(native_renderer)->onStop();
}

VRO_METHOD(void, nativeSetSceneController)(VRO_ARGS
                                           jlong native_renderer,
                                           jlong native_scene_controller_ref) {
    if (kRunRendererTest) {
        return;
    }

    std::weak_ptr<VROSceneRenderer> renderer_w = Renderer::native(native_renderer);
    std::weak_ptr<VROSceneController> sceneController_w = VRO_REF_GET(VROSceneController, native_scene_controller_ref);

    VROPlatformDispatchAsyncRenderer([renderer_w, sceneController_w] {
        std::shared_ptr<VROSceneRenderer> renderer = renderer_w.lock();
        if (!renderer) {
            return;
        }
        std::shared_ptr<VROSceneController> sceneController = sceneController_w.lock();
        if (!sceneController) {
            return;
        }
        renderer->setSceneController(sceneController);
    });
}

VRO_METHOD(void, nativeSetSceneControllerWithAnimation)(VRO_ARGS
                                                        jlong native_renderer,
                                                        jlong native_scene_controller_ref,
                                                        VRO_FLOAT duration) {
    if (kRunRendererTest) {
        return;
    }

    std::weak_ptr<VROSceneRenderer> renderer_w = Renderer::native(native_renderer);
    std::weak_ptr<VROSceneController> sceneController_w = VRO_REF_GET(VROSceneController, native_scene_controller_ref);

    VROPlatformDispatchAsyncRenderer([renderer_w, sceneController_w, duration] {
        std::shared_ptr<VROSceneRenderer> renderer = renderer_w.lock();
        if (!renderer) {
            return;
        }
        std::shared_ptr<VROSceneController> sceneController = sceneController_w.lock();
        if (!sceneController) {
            return;
        }
        renderer->setSceneController(sceneController, duration, VROTimingFunctionType::EaseOut);
    });
}

VRO_METHOD(void, nativeSetPointOfView)(VRO_ARGS
                                       jlong native_renderer,
                                       jlong native_node_ref) {
    std::weak_ptr<VROSceneRenderer> renderer_w = Renderer::native(native_renderer);
    std::shared_ptr<VRONode> node;
    if (native_node_ref != 0) {
        node = VRO_REF_GET(VRONode, native_node_ref);
    }

    VROPlatformDispatchAsyncRenderer([renderer_w, node] {
        std::shared_ptr<VROSceneRenderer> renderer = renderer_w.lock();
        if (!renderer) {
            return;
        }

        if (!node) {
            renderer->getRenderer()->setPointOfView(nullptr);
        }
        else {
            renderer->getRenderer()->setPointOfView(node);
        }
    });
}

VRO_METHOD(void, nativeOnSurfaceCreated)(VRO_ARGS
                                         jobject surface,
                                         jlong native_renderer) {

    Renderer::native(native_renderer)->onSurfaceCreated(surface);
}

VRO_METHOD(void, nativeOnSurfaceChanged)(VRO_ARGS
                                         jobject surface,
                                         VRO_INT width,
                                         VRO_INT height,
                                         jlong native_renderer) {
    Renderer::native(native_renderer)->onSurfaceChanged(surface, width, height);
}

VRO_METHOD(void, nativeOnSurfaceDestroyed)(VRO_ARGS
                                           jlong native_renderer) {
    Renderer::native(native_renderer)->onSurfaceDestroyed();
}

VRO_METHOD(VRO_STRING, nativeGetHeadset)(VRO_ARGS
                                         jlong nativeRenderer) {
    std::string headset = Renderer::native(nativeRenderer)->getRenderer()->getInputController()->getHeadset();
    return VRO_NEW_STRING(headset.c_str());
}

VRO_METHOD(VRO_STRING, nativeGetController)(VRO_ARGS
                                            jlong nativeRenderer) {
    std::string controller = Renderer::native(nativeRenderer)->getRenderer()->getInputController()->getController();
    return VRO_NEW_STRING(controller.c_str());
}

VRO_METHOD(void, nativeSetDebugHUDEnabled)(VRO_ARGS
                                           jlong native_renderer,
                                           jboolean enabled) {
    std::shared_ptr<VROSceneRenderer> renderer = Renderer::native(native_renderer);
    renderer->getRenderer()->setDebugHUDEnabled(enabled);
}

// This function is OVR only!
VRO_METHOD(void, nativeRecenterTracking)(VRO_ARGS
                                         jlong native_renderer) {
    std::shared_ptr<VROSceneRenderer> renderer = Renderer::native(native_renderer);
    std::shared_ptr<VROSceneRendererOVR> ovrRenderer = std::dynamic_pointer_cast<VROSceneRendererOVR>(renderer);
    ovrRenderer->recenterTracking();
}

VRO_METHOD(VRO_FLOAT_ARRAY, nativeProjectPoint)(VRO_ARGS
                                                jlong renderer_j,
                                                VRO_FLOAT x, VRO_FLOAT y, VRO_FLOAT z) {
    std::shared_ptr<VRORenderer> renderer = Renderer::native(renderer_j)->getRenderer();
    return ARUtilsCreateFloatArrayFromVector3f(renderer->projectPoint({ x, y, z }));
}

VRO_METHOD(VRO_FLOAT_ARRAY, nativeUnprojectPoint)(VRO_ARGS
                                                  jlong renderer_j,
                                                  VRO_FLOAT x, VRO_FLOAT y, VRO_FLOAT z) {
    std::shared_ptr<VRORenderer> renderer = Renderer::native(renderer_j)->getRenderer();
    return ARUtilsCreateFloatArrayFromVector3f(renderer->unprojectPoint({ x, y, z }));
}

VRO_METHOD(void, nativeSetClearColor)(VRO_ARGS
                                      jlong native_renderer,
                                      VRO_INT color) {
    std::weak_ptr<VROSceneRenderer> renderer_w = Renderer::native(native_renderer);
    VROPlatformDispatchAsyncRenderer([renderer_w, color] {
        std::shared_ptr<VROSceneRenderer> renderer = renderer_w.lock();
        if (!renderer) {
            return;
        }
        // Get the color
        float a = ((color >> 24) & 0xFF) / 255.0f;
        float r = ((color >> 16) & 0xFF) / 255.0f;
        float g = ((color >> 8) & 0xFF) / 255.0f;
        float b = (color & 0xFF) / 255.0f;

        VROVector4f vecColor(r, g, b, a);
        renderer->setClearColor(vecColor);
    });
}

VRO_METHOD(VRO_REF(VROFrameListener), nativeCreateFrameListener)(VRO_ARGS
                                                                 jlong native_renderer) {
    std::shared_ptr<FrameListenerJNI> listener = std::make_shared<FrameListenerJNI>(obj);
    return VRO_REF_NEW(VROFrameListener, listener);
}

VRO_METHOD(void, nativeDestroyFrameListener)(VRO_ARGS
                                             jlong frame_listener) {
    delete reinterpret_cast<PersistentRef<VROFrameListener> *>(frame_listener);
}

VRO_METHOD(void, nativeAddFrameListener)(VRO_ARGS
                                         jlong native_renderer, jlong frame_listener) {

    std::weak_ptr<VROSceneRenderer> renderer_w = Renderer::native(native_renderer);
    std::weak_ptr<VROFrameListener> frameListener_w = VRO_REF_GET(VROFrameListener, frame_listener);

    VROPlatformDispatchAsyncRenderer([renderer_w, frameListener_w] {
        std::shared_ptr<VROSceneRenderer> renderer = renderer_w.lock();
        if (!renderer) {
            return;
        }

        std::shared_ptr<VROFrameListener> frameListener = frameListener_w.lock();
        if (!frameListener) {
            return;
        }
        renderer->getFrameSynchronizer()->addFrameListener(frameListener);
    });
}

VRO_METHOD(void, nativeRemoveFrameListener)(VRO_ARGS
                                            jlong native_renderer, jlong frame_listener) {
    std::weak_ptr<VROSceneRenderer> renderer_w = Renderer::native(native_renderer);
    std::weak_ptr<VROFrameListener> frameListener_w  = VRO_REF_GET(VROFrameListener, frame_listener);

    VROPlatformDispatchAsyncRenderer([renderer_w, frameListener_w] {
        std::shared_ptr<VROSceneRenderer> renderer = renderer_w.lock();
        if (!renderer) {
            return;
        }

        std::shared_ptr<VROFrameListener> frameListener = frameListener_w.lock();
        if (!frameListener) {
            return;
        }

        renderer->getFrameSynchronizer()->removeFrameListener(frameListener);
    });
}

VRO_METHOD(jboolean, nativeIsReticlePointerFixed)(VRO_ARGS
                                                  jlong native_renderer) {
    std::shared_ptr<VROSceneRenderer> sceneRenderer = Renderer::native(native_renderer);
    return sceneRenderer->getRenderer()->getInputController()->getPresenter()->getReticle()->isHeadlocked();
}

VRO_METHOD(VRO_FLOAT_ARRAY, nativeGetCameraPositionRealtime)(VRO_ARGS
                                                             jlong native_renderer) {
    std::shared_ptr<VROSceneRenderer> renderer = Renderer::native(native_renderer);
    return ARUtilsCreateFloatArrayFromVector3f(renderer->getRenderer()->getCameraPositionRealTime());
}

VRO_METHOD(VRO_FLOAT_ARRAY, nativeGetCameraRotationRealtime)(VRO_ARGS
                                                             jlong native_renderer) {
    std::shared_ptr<VROSceneRenderer> renderer = Renderer::native(native_renderer);
    return ARUtilsCreateFloatArrayFromVector3f(renderer->getRenderer()->getCameraRotationRealTime());
}

VRO_METHOD(VRO_FLOAT_ARRAY, nativeGetCameraForwardRealtime)(VRO_ARGS
                                                            jlong native_renderer) {
    std::shared_ptr<VROSceneRenderer> renderer = Renderer::native(native_renderer);
    return ARUtilsCreateFloatArrayFromVector3f(renderer->getRenderer()->getCameraForwardRealTime());
}

VRO_METHOD(VRO_FLOAT, nativeGetFieldOfView)(VRO_ARGS
                                            jlong native_ref) {
    std::shared_ptr<VROSceneRenderer> renderer = Renderer::native(native_ref);
    return renderer->getRenderer()->getActiveFieldOfView();
}

VRO_METHOD(void, nativeSetCameraListener)(VRO_ARGS
                                          jlong native_renderer,
                                          jboolean enabled) {
    std::shared_ptr<VROSceneRenderer> renderer = Renderer::native(native_renderer);
    if (enabled) {
        std::shared_ptr<CameraDelegateJNI> listener = std::make_shared<CameraDelegateJNI>(obj);
        renderer->getRenderer()->setCameraDelegate(listener);
    } else {
        renderer->getRenderer()->setCameraDelegate(nullptr);
    }
}

VRO_METHOD(void, nativeSetShadowsEnabled)(VRO_ARGS
                                          jlong native_renderer,
                                          jboolean enabled) {
    std::weak_ptr<VROSceneRenderer> sceneRenderer_w = Renderer::native(native_renderer);

    VROPlatformDispatchAsyncRenderer([sceneRenderer_w, enabled] {
        std::shared_ptr<VROSceneRenderer> sceneRenderer = sceneRenderer_w.lock();
        if (!sceneRenderer) {
            return;
        }
        sceneRenderer->getRenderer()->setShadowsEnabled(enabled);
    });
}

VRO_METHOD(void, nativeSetHDREnabled)(VRO_ARGS
                                      jlong native_renderer,
                                      jboolean enabled) {
    std::weak_ptr<VROSceneRenderer> sceneRenderer_w = Renderer::native(native_renderer);

    VROPlatformDispatchAsyncRenderer([sceneRenderer_w, enabled] {
        std::shared_ptr<VROSceneRenderer> sceneRenderer = sceneRenderer_w.lock();
        if (!sceneRenderer) {
            return;
        }
        sceneRenderer->getRenderer()->setHDREnabled(enabled);
    });
}

VRO_METHOD(void, nativeSetPBREnabled)(VRO_ARGS
                                      jlong native_renderer,
                                      jboolean enabled) {
    std::weak_ptr<VROSceneRenderer> sceneRenderer_w = Renderer::native(native_renderer);

    VROPlatformDispatchAsyncRenderer([sceneRenderer_w, enabled] {
        std::shared_ptr<VROSceneRenderer> sceneRenderer = sceneRenderer_w.lock();
        if (!sceneRenderer) {
            return;
        }
        sceneRenderer->getRenderer()->setPBREnabled(enabled);
    });
}

VRO_METHOD(void, nativeSetBloomEnabled)(VRO_ARGS
                                        jlong native_renderer,
                                        jboolean enabled) {
    std::weak_ptr<VROSceneRenderer> sceneRenderer_w = Renderer::native(native_renderer);

    VROPlatformDispatchAsyncRenderer([sceneRenderer_w, enabled] {
        std::shared_ptr<VROSceneRenderer> sceneRenderer = sceneRenderer_w.lock();
        if (!sceneRenderer) {
            return;
        }
        sceneRenderer->getRenderer()->setBloomEnabled(enabled);
    });
}


void invokeHitTestResultsCallback(std::vector<VROHitTestResult> &results, jweak weakCallback) {
    JNIEnv *env = VROPlatformGetJNIEnv();
    jclass hitTestResultClass = env->FindClass("com/viro/core/HitTestResult");

    jobjectArray resultsArray = env->NewObjectArray(results.size(), hitTestResultClass, NULL);
    for (int i = 0; i < results.size(); i++) {
        jobject result = ARUtilsCreateHitTestResult(results[i]);
        env->SetObjectArrayElement(resultsArray, i, result);
    }

    jobject globalArrayRef = env->NewGlobalRef(resultsArray);
    VROPlatformDispatchAsyncApplication([weakCallback, globalArrayRef] {
        JNIEnv *env = VROPlatformGetJNIEnv();
        jobject callback = env->NewLocalRef(weakCallback);
        VROPlatformCallHostFunction(callback, "onHitTestFinished",
                                    "([Lcom/viro/core/HitTestResult;)V",
                                    globalArrayRef);
        env->DeleteGlobalRef(globalArrayRef);
        env->DeleteWeakGlobalRef(weakCallback);
    });
}


void invokeEmptyHitTestResultsCallback(jweak weakCallback) {
    VROPlatformDispatchAsyncApplication([weakCallback] {
        JNIEnv *env = VROPlatformGetJNIEnv();
        jobject callback = env->NewLocalRef(weakCallback);
        jclass arHitTestResultClass = env->FindClass("com/viro/core/HitTestResult");
        jobjectArray emptyArray = env->NewObjectArray(0, arHitTestResultClass, NULL);
        VROPlatformCallHostFunction(callback, "onHitTestFinished",
                                    "([Lcom/viro/core/HitTestResult;)V", emptyArray);
        env->DeleteWeakGlobalRef(weakCallback);
    });
}

void performHitTestRay(VROVector3f origin, VROVector3f ray, bool boundsOnly, std::weak_ptr<VROSceneRenderer> renderer_w,
                       jweak weakCallback) {
    std::shared_ptr<VROSceneRenderer> renderer = renderer_w.lock();
    if (!renderer) {
        invokeEmptyHitTestResultsCallback(weakCallback);
    }
    else {
        std::vector<VROHitTestResult> results = renderer->performHitTest(origin, ray, boundsOnly);
        invokeHitTestResultsCallback(results, weakCallback);
    }
}

void performHitTestPoint(float x, float y, bool boundsOnly,  std::weak_ptr<VROSceneRenderer> renderer_w,
                         jweak weakCallback) {
    std::shared_ptr<VROSceneRenderer> renderer = renderer_w.lock();
    if (!renderer) {
        invokeEmptyHitTestResultsCallback(weakCallback);
    }
    else {
        std::vector<VROHitTestResult> results = renderer->performHitTest(x, y, boundsOnly);
        invokeHitTestResultsCallback(results, weakCallback);
    }
}


VRO_METHOD(void, nativePerformHitTestWithPoint) (VRO_ARGS
                                                   VRO_LONG native_renderer,
                                                   VRO_INT x, VRO_INT y, VRO_BOOL boundsOnly,
                                                   VRO_OBJECT callback) {
    std::weak_ptr<VROSceneRenderer> renderer_w = Renderer::native(native_renderer);
    if(callback == VRO_OBJECT_NULL) {
        pinfo("TEST_nativePerformHitTestWithPoint IS NULL!!");
    }

    jweak weakCallback = env->NewWeakGlobalRef(callback);
    VROPlatformDispatchAsyncRenderer([env, renderer_w, boundsOnly, weakCallback, x, y] {
        std::shared_ptr<VROSceneRenderer> sceneRenderer = renderer_w.lock();
        performHitTestPoint(x, y, boundsOnly, sceneRenderer, weakCallback);
    });
}

VRO_METHOD(void, nativePerformHitTestWithRay) (VRO_ARGS
                                                       VRO_LONG native_renderer,
                                                       VRO_FLOAT_ARRAY origin,
                                                       VRO_FLOAT_ARRAY ray,
                                                       VRO_BOOL boundsOnly,
                                                       VRO_OBJECT callback) {
    // Grab ray origin to perform the  hit test
    VRO_FLOAT *originArray = VRO_FLOAT_ARRAY_GET_ELEMENTS(origin);
    VROVector3f originVec = VROVector3f(originArray[0], originArray[1], originArray[2]);
    VRO_FLOAT_ARRAY_RELEASE_ELEMENTS(origin, originArray);

    // Grab ray destination to perform the AR hit test
    VRO_FLOAT *rayArray = VRO_FLOAT_ARRAY_GET_ELEMENTS(ray);
    VROVector3f rayVec = VROVector3f(rayArray[0], rayArray[1], rayArray[2]);
    VRO_FLOAT_ARRAY_RELEASE_ELEMENTS(ray, rayArray);

    // Create weak pointers for dispatching
    std::weak_ptr<VROSceneRenderer> renderer_w = Renderer::native(native_renderer);
    jweak weakCallback = env->NewWeakGlobalRef(callback);

    VROPlatformDispatchAsyncRenderer([renderer_w, boundsOnly, weakCallback, originVec, rayVec] {
        performHitTestRay(originVec, rayVec, boundsOnly, renderer_w, weakCallback);
    });
}


}  // extern "C"
