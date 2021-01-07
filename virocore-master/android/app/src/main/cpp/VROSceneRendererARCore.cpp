//
//  VROSceneRendererARCore.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 9/27/178.
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

#include "VROSceneRendererARCore.h"

#include <android/log.h>
#include <assert.h>
#include <stdlib.h>
#include <cmath>
#include <random>
#include <VROTime.h>
#include <VROProjector.h>
#include <VROARHitTestResult.h>
#include <VROInputControllerAR.h>
#include "VROARCamera.h"
#include "arcore/VROARSessionARCore.h"
#include "arcore/VROARFrameARCore.h"
#include "arcore/VROARCameraARCore.h"

#include "VRODriverOpenGLAndroid.h"
#include "VROGVRUtil.h"
#include "VRONodeCamera.h"
#include "VROMatrix4f.h"
#include "VROViewport.h"
#include "VRORenderer.h"
#include "VROSurface.h"
#include "VRONode.h"
#include "VROARScene.h"
#include "VROInputControllerCardboard.h"
#include "VROAllocationTracker.h"
#include "VROInputControllerARAndroid.h"

static VROVector3f const kZeroVector = VROVector3f();

#pragma mark - Setup

VROSceneRendererARCore::VROSceneRendererARCore(VRORendererConfiguration config,
                                               std::shared_ptr<gvr::AudioApi> gvrAudio) :
    _arcoreInstalled(false),
    _destroyed(false) {

    _driver = std::make_shared<VRODriverOpenGLAndroid>(gvrAudio);
    _session = std::make_shared<VROARSessionARCore>(_driver);

    // instantiate the input controller w/ viewport size (0,0) and update it later.
    std::shared_ptr<VROInputControllerAR> controller = std::make_shared<VROInputControllerARAndroid>(0, 0, _driver);

    _renderer = std::make_shared<VRORenderer>(config, controller);
    controller->setSession(_session);

    _pointOfView = std::make_shared<VRONode>();
    _pointOfView->setCamera(std::make_shared<VRONodeCamera>());
    _renderer->setPointOfView(_pointOfView);
}

VROSceneRendererARCore::~VROSceneRendererARCore() {

}

#pragma mark - Rendering

void VROSceneRendererARCore::initGL() {

}

GLuint VROSceneRendererARCore::getCameraTextureId() const {
    return _session->getCameraTextureId();
}

void VROSceneRendererARCore::setARCoreSession(arcore::Session *session) {
    _arcoreInstalled = true;
    _session->setARCoreSession(session, getFrameSynchronizer());
}

void VROSceneRendererARCore::onDrawFrame() {
    if (_destroyed) {
        return;
    }

    if (_arcoreInstalled) {
        renderFrame();
    } else {
        renderNothing();
    }

    ++_frame;
    ALLOCATION_TRACKER_PRINT();
}

void VROSceneRendererARCore::renderFrame() {
    /*
     Setup GL state.
     */
    glEnable(GL_DEPTH_TEST);
    _driver->setCullMode(VROCullMode::Back);

    // Attempt to initialize the ARSession if we have not yet done so.
    VROViewport viewport(0, 0, _surfaceSize.width, _surfaceSize.height);
    bool backgroundNeedsReset = false;
    if (_sceneController) {
        if (!_cameraBackground) {
            initARSession(viewport, _sceneController->getScene());
            backgroundNeedsReset = true;
        }
    }

    // If the ARSession is not yet ready, render black
    if (!_session->isReady()){
        renderWaitingForTracking(viewport);
        return;
    }

    _session->setViewport(viewport);
    std::unique_ptr<VROARFrame> &frame = _session->updateFrame();
    updateARBackground(frame, backgroundNeedsReset);

    // Notify the current ARScene with the ARCamera's tracking state.
    const std::shared_ptr<VROARCamera> camera = frame->getCamera();
    if (_sceneController) {
        std::shared_ptr<VROARScene> arScene = std::dynamic_pointer_cast<VROARScene>(_sceneController->getScene());
        passert_msg (arScene != nullptr, "AR View requires an AR Scene!");
        arScene->setTrackingState(camera->getTrackingState(),
                                  camera->getLimitedTrackingStateReason(), false);
    }

    /*
     If we attempt to get the projection matrix from the session before tracking has
     resumed (even if the session itself has been resumed) we'll get a SessionPausedException.
     Protect against this by not accessing the session until tracking is operational.
     */
    if (camera->getTrackingState() == VROARTrackingState::Normal) {
        renderWithTracking(camera, frame, viewport);
    } else {
        renderWaitingForTracking(viewport);
    }
}

void VROSceneRendererARCore::updateARBackground(std::unique_ptr<VROARFrame> &frame,
                                                bool forceReset) {
    // Only update the rendered camera background if need be.
    if (!forceReset && !((VROARFrameARCore *) frame.get())->hasDisplayGeometryChanged()) {
        return;
    }

    VROVector3f BL, BR, TL, TR;
    ((VROARFrameARCore *)frame.get())->getBackgroundTexcoords(&BL, &BR, &TL, &TR);

    _cameraBackground->setTextureCoordinates(BL, BR, TL, TR);

    // Wait until we have these proper texture coordinates before installing the background
    if (!_sceneController->getScene()->getRootNode()->getBackground()) {
        _sceneController->getScene()->getRootNode()->setBackground(_cameraBackground);
    }
}

void VROSceneRendererARCore::renderWithTracking(const std::shared_ptr<VROARCamera> &camera,
                                                const std::unique_ptr<VROARFrame> &frame,
                                                VROViewport viewport) {
    VROFieldOfView fov;
    VROMatrix4f projection = camera->getProjection(viewport, kZNear, _renderer->getFarClippingPlane(), &fov);
    VROMatrix4f rotation = camera->getRotation();
    VROVector3f position = camera->getPosition();

    /*
     Render the 3D scene.
     */
    _pointOfView->getCamera()->setPosition(position);
    _renderer->prepareFrame(_frame, viewport, fov, rotation, projection, _driver);
    _renderer->renderEye(VROEyeType::Monocular, _renderer->getLookAtMatrix(), projection, viewport, _driver);
    _renderer->renderHUD(VROEyeType::Monocular, VROMatrix4f::identity(), projection, _driver);
    _renderer->endFrame(_driver);

    /*
     Notify scene of the updated ambient light estimates
     */
    std::shared_ptr<VROARScene> scene = std::dynamic_pointer_cast<VROARScene>(_session->getScene());
    scene->updateAmbientLight(frame->getAmbientLightIntensity(), frame->getAmbientLightColor());
}

void VROSceneRendererARCore::renderWaitingForTracking(VROViewport viewport) {
    /*
     Render black while waiting for the AR session to initialize.
     */
    VROFieldOfView fov = _renderer->computeUserFieldOfView(viewport.getWidth(), viewport.getHeight());
    VROMatrix4f projection = fov.toPerspectiveProjection(kZNear, _renderer->getFarClippingPlane());

    _renderer->prepareFrame(_frame, viewport, fov, VROMatrix4f::identity(), projection, _driver);
    _renderer->renderEye(VROEyeType::Monocular, _renderer->getLookAtMatrix(), projection, viewport, _driver);
    _renderer->renderHUD(VROEyeType::Monocular, VROMatrix4f::identity(), projection, _driver);
    _renderer->endFrame(_driver);
}

void VROSceneRendererARCore::renderNothing() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void VROSceneRendererARCore::initARSession(VROViewport viewport, std::shared_ptr<VROScene> scene) {
    // Create the background surface
    _cameraBackground = VROSurface::createSurface(viewport.getX() + viewport.getWidth() / 2.0,
                                                  viewport.getY() + viewport.getHeight() / 2.0,
                                                  viewport.getWidth(), viewport.getHeight(),
                                                  0, 0, 1, 1);
    _cameraBackground->setScreenSpace(true);
    _cameraBackground->setName("Camera");

    // Initialize the background texture in the session
    _session->initCameraTexture(_driver);

    // Assign the background texture to the background surface
    std::shared_ptr<VROMaterial> material = _cameraBackground->getMaterials()[0];
    material->setLightingModel(VROLightingModel::Constant);
    material->getDiffuse().setTexture(_session->getCameraBackgroundTexture());
    material->setWritesToDepthBuffer(false);
    material->setNeedsToneMapping(false);

    std::shared_ptr<VROARScene> arScene = std::dynamic_pointer_cast<VROARScene>(scene);
    passert_msg (arScene != nullptr, "AR View requires an AR Scene!");
    arScene->setDriver(_driver);
    arScene->setARSession(_session);

    _session->setScene(scene);
    _session->setViewport(viewport);
    _session->setDelegate(arScene->getSessionDelegate());
    _session->run();

    arScene->addNode(_pointOfView);
}

/*
 Update render sizes as the surface changes.
 */
void VROSceneRendererARCore::onSurfaceChanged(jobject surface, VRO_INT width, VRO_INT height) {
    VROThreadRestricted::setThread(VROThreadName::Renderer);

    _surfaceSize.width = width;
    _surfaceSize.height = height;

    if (_cameraBackground) {
        _cameraBackground->setX(width / 2.0);
        _cameraBackground->setY(height / 2.0);
        _cameraBackground->setWidth(width);
        _cameraBackground->setHeight(height);
    }

    std::shared_ptr<VROInputControllerARAndroid> inputControllerAR =
            std::dynamic_pointer_cast<VROInputControllerARAndroid>(_renderer->getInputController());

    if (inputControllerAR) {
        inputControllerAR->setViewportSize(width, height);
    }
}

void VROSceneRendererARCore::onTouchEvent(int action, float x, float y) {
    std::shared_ptr<VROInputControllerBase> baseController = _renderer->getInputController();
    std::shared_ptr<VROInputControllerARAndroid> arTouchController
            = std::dynamic_pointer_cast<VROInputControllerARAndroid>(baseController);
    arTouchController->onTouchEvent(action, x, y);
}

void VROSceneRendererARCore::onPinchEvent(int pinchState, float scaleFactor,
                                          float viewportX, float viewportY) {
    std::shared_ptr<VROInputControllerBase> baseController = _renderer->getInputController();
    std::shared_ptr<VROInputControllerARAndroid> arTouchController
            = std::dynamic_pointer_cast<VROInputControllerARAndroid>(baseController);
    arTouchController->onPinchEvent(pinchState, scaleFactor, viewportX, viewportY);
}

void VROSceneRendererARCore::onRotateEvent(int rotateState, float rotateRadians, float viewportX,
                                           float viewportY) {
    std::shared_ptr<VROInputControllerBase> baseController = _renderer->getInputController();
    std::shared_ptr<VROInputControllerARAndroid> arTouchController
            = std::dynamic_pointer_cast<VROInputControllerARAndroid>(baseController);
    arTouchController->onRotateEvent(rotateState, rotateRadians, viewportX, viewportY);
}

void VROSceneRendererARCore::onPause() {
    _session->pause();

    std::shared_ptr<VROSceneRendererARCore> shared = shared_from_this();
    VROPlatformDispatchAsyncRenderer([shared] {
        shared->_renderer->getInputController()->onPause();
        shared->_driver->pause();
    });
}

void VROSceneRendererARCore::onResume() {
    _session->run();
    std::shared_ptr<VROSceneRendererARCore> shared = shared_from_this();

    VROPlatformDispatchAsyncRenderer([shared] {
        shared->_renderer->getInputController()->onResume();
        shared->_driver->resume();
    });
}

void VROSceneRendererARCore::onDestroy() {
    _destroyed = true;
}

void VROSceneRendererARCore::setVRModeEnabled(bool enabled) {

}

void VROSceneRendererARCore::setSceneController(std::shared_ptr<VROSceneController> sceneController) {
    _sceneController = sceneController;

    std::shared_ptr<VROARScene> arScene = std::dynamic_pointer_cast<VROARScene>(sceneController->getScene());
    passert_msg(arScene != nullptr, "[Viro] AR requires using ARScene");

    // Detection types default to empty. This way we can use the default specified by VROARScene,
    // and only override that value is detectionTypes are explicitly set here in the VROARSceneRenderer
    if (!_detectionTypes.empty()) {
        arScene->setAnchorDetectionTypes(_detectionTypes);
    }
    VROSceneRenderer::setSceneController(sceneController);

    // Reset the camera background for the new scene
    _cameraBackground.reset();
}

void VROSceneRendererARCore::setSceneController(std::shared_ptr<VROSceneController> sceneController, float seconds,
                        VROTimingFunctionType timingFunction) {

    _sceneController = sceneController;
    std::shared_ptr<VROARScene> arScene = std::dynamic_pointer_cast<VROARScene>(sceneController->getScene());
    passert_msg(arScene != nullptr, "[Viro] AR requires using ARScene");

    // Detection types default to empty. This way we can use the default specified by VROARScene,
    // and only override that value is detectionTypes are explicitly set here in the VROARSceneRenderer
    if (!_detectionTypes.empty()) {
        arScene->setAnchorDetectionTypes(_detectionTypes);
    }
    VROSceneRenderer::setSceneController(sceneController, seconds, timingFunction);

    // Reset the camera background for the new scene
    _cameraBackground.reset();
}

std::vector<std::shared_ptr<VROARHitTestResult>> VROSceneRendererARCore::performARHitTest(float x, float y) {
    int viewportArr[4] = {0, 0, _surfaceSize.width, _surfaceSize.height};

    std::unique_ptr<VROARFrame> &frame = _session->getLastFrame();
    if (frame && x >= 0 && x <= viewportArr[2] && y >= 0 && y <= viewportArr[3]) {
        return frame->hitTest(x, y, { VROARHitTestResultType::ExistingPlaneUsingExtent,
                                      VROARHitTestResultType::ExistingPlane,
                                      VROARHitTestResultType::EstimatedHorizontalPlane,
                                      VROARHitTestResultType::FeaturePoint });
    };
    return {};
}

std::vector<std::shared_ptr<VROARHitTestResult>> VROSceneRendererARCore::performARHitTest(VROVector3f rayOrigin, VROVector3f rayDestination) {
    std::unique_ptr<VROARFrame> &frame = _session->getLastFrame();
    if (frame) {
        return frame->hitTestRay(&rayOrigin, &rayDestination, { VROARHitTestResultType::ExistingPlaneUsingExtent,
                                                           VROARHitTestResultType::ExistingPlane,
                                                           VROARHitTestResultType::EstimatedHorizontalPlane,
                                                           VROARHitTestResultType::FeaturePoint });
    } else {
        return {};
    }
}

std::vector<std::shared_ptr<VROARHitTestResult>> VROSceneRendererARCore::performARHitTest(VROVector3f ray) {
    VROVector3f cameraForward = getRenderer()->getCamera().getForward();
    if (cameraForward.dot(ray) <= 0) {
        return {};
    }

    VROVector3f worldPoint = getRenderer()->getCamera().getPosition() + ray.normalize();
    VROVector3f screenPoint = _renderer->projectPoint(worldPoint);
    return performARHitTest(screenPoint.x, screenPoint.y);
}

void VROSceneRendererARCore::setDisplayGeometry(int rotation, int width, int height) {
    _session->setDisplayGeometry((VROARDisplayRotation) rotation, width, height);
}

void VROSceneRendererARCore::setCameraAutoFocusEnabled(bool enabled) {
    _session->setAutofocus(enabled);
}

bool VROSceneRendererARCore::isCameraAutoFocusEnabled() {
    return _session->isCameraAutoFocusEnabled();
}

void VROSceneRendererARCore::setAnchorDetectionTypes(std::set<VROAnchorDetection> types) {
    _detectionTypes = types;

    if (_sceneController) {
        std::shared_ptr<VROARScene> scene = std::dynamic_pointer_cast<VROARScene>(_sceneController->getScene());
        if (scene) {
            scene->setAnchorDetectionTypes(_detectionTypes);
        }
    }
}

void VROSceneRendererARCore::enableTracking(bool shouldTrack) {

}


