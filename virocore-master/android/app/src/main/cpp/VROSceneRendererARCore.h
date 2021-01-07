//
//  VROSceneRendererARCore.h
//  ViroRenderer
//
//  Created by Raj Advani on 9/27/17.
//  Copyright © 2017 Viro Media. All rights reserved.
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

#ifndef VRO_SCENE_RENDERER_ARCORE_H_  // NOLINT
#define VRO_SCENE_RENDERER_ARCORE_H_  // NOLINT

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <jni.h>

#include <string>
#include <thread>  // NOLINT
#include <vector>
#include <arcore/VROARSessionARCore.h>
#include "VROSceneRenderer.h"
#include "VRODriverOpenGLAndroid.h"

#include "vr/gvr/capi/include/gvr_audio.h"
#include "vr/gvr/capi/include/gvr_types.h"

class VROSurface;
class VROARCamera;
class VROARFrame;
class VRORendererConfiguration;
class VROARSessionARCore;

class VROSceneRendererARCore : public VROSceneRenderer, public std::enable_shared_from_this<VROSceneRendererARCore> {

public:

    /*
     Create a VROSceneRendererARCore.

     @param gvr_audio_api The (owned) gvr::AudioApi context.
     */
    VROSceneRendererARCore(VRORendererConfiguration config,
                           std::shared_ptr<gvr::AudioApi> gvrAudio);
    virtual ~VROSceneRendererARCore();

    /*
     Inherited from VROSceneRenderer.
     */
    void initGL();
    void onDrawFrame();
    void onTouchEvent(int action, float x, float y);
    void onKeyEvent(int keyCode, int action) {} // Not Required
    void onPinchEvent(int pinchState, float scaleFactor, float viewportX, float viewportY);
    void onRotateEvent(int rotateState, float rotateRadians, float viewportX, float viewportY);

    void setVRModeEnabled(bool enabled);

    /*
     Override so that this object can hold onto the VROSceneController as
     well.
    */
    void setSceneController(std::shared_ptr<VROSceneController> sceneController);
    void setSceneController(std::shared_ptr<VROSceneController> sceneController, float seconds,
                            VROTimingFunctionType timingFunction);
    std::shared_ptr<VROSceneController> getSceneController() { return _sceneController; };

    /*
     Activity lifecycle.
     */
    void onStart() {}
    void onPause();
    void onResume();
    void onStop() {}
    void onDestroy();

    /*
     Surface lifecycle.
     */
    void onSurfaceCreated(jobject surface) {}
    void onSurfaceChanged(jobject surface, VRO_INT width, VRO_INT height);
    void onSurfaceDestroyed() {}

    /*
     Set to true when ARCore is installed. Unlocks the renderer.
     */
    void setARCoreSession(arcore::Session *session);

    /*
     Retrieves the texture ID used for the camera background.
     */
    GLuint getCameraTextureId() const;

    /*
     AR hit test using point on the screen in 2D coordinate system.
     */
    std::vector<std::shared_ptr<VROARHitTestResult>> performARHitTest(float x, float y);

    /*
     AR hit test using a ray from camera's position into the 3D scene.
     */
    std::vector<std::shared_ptr<VROARHitTestResult>> performARHitTest(VROVector3f ray);

    /*
     AR hit test using a ray from origin to destination in the 3D scene.
    */
    std::vector<std::shared_ptr<VROARHitTestResult>> performARHitTest(VROVector3f rayOrigin, VROVector3f rayDestination);
    /*
     Set the size of the parent view holding the AR screen. Invoked from ViroViewARCore.
     */
    void setDisplayGeometry(int rotation, int width, int height);

    /*
     * Set camera's ArFocusMode as AUTO_FOCUS if enabled is true, else set to FIXED_FOCUS
     */
    void setCameraAutoFocusEnabled(bool enabled);

    /*
     * Return true if camera's ArFocusMode is set to AUTO_FOCUS;
     */
    bool isCameraAutoFocusEnabled();

    /*
     Set the anchor detection mode used by ARCore.
     */
    void setAnchorDetectionTypes(std::set<VROAnchorDetection> types);

    /*
     This is a function that enables/disables tracking (for debug purposes!)
     */
    void enableTracking(bool shouldTrack);

private:

    void renderFrame();
    void renderWithTracking(const std::shared_ptr<VROARCamera> &camera, const std::unique_ptr<VROARFrame> &frame,
                            VROViewport viewport);
    void updateARBackground(std::unique_ptr<VROARFrame> &frame, bool forceReset);
    void renderWaitingForTracking(VROViewport viewport);
    void renderNothing();
    void initARSession(VROViewport viewport, std::shared_ptr<VROScene> scene);

    std::shared_ptr<VROSurface> _cameraBackground;
    gvr::Sizei _surfaceSize;
    bool _arcoreInstalled;
    bool _destroyed;

    // Detection types are only stored here so that they can be pushed to the ARScene when that
    // is injected into the scene renderer (from there they are pushed into the VROARSession).
    std::set<VROAnchorDetection> _detectionTypes;

    std::shared_ptr<VRONode> _pointOfView;
    std::shared_ptr<VROARSessionARCore> _session;
};

#endif  // VRO_SCENE_RENDERER_ARCORE_H  // NOLINT
