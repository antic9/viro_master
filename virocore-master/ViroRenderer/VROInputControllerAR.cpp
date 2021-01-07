//
//  VROInputControllerAR.cpp
//  ViroKit
//
//  Created by Andy Chu on 6/21/17.
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

#include "VROInputControllerAR.h"
#include "VRORenderer.h"
#include "VROProjector.h"
#include "VROARFrame.h"
#include "VROARCamera.h"

VROInputControllerAR::VROInputControllerAR(float viewportWidth, float viewportHeight,
                                           std::shared_ptr<VRODriver> driver) :
    VROInputControllerBase(driver),
    _viewportWidth(viewportWidth),
    _viewportHeight(viewportHeight),
    _isTouchOngoing(false),
    _isPinchOngoing(false),
    _isRotateOngoing(false),
    _lastProcessDragTimeMillis(0){
}

VROVector3f VROInputControllerAR::getDragForwardOffset() {
    return VROVector3f();
}

void VROInputControllerAR::onProcess(const VROCamera &camera) {
    _latestCamera = camera;
    processTouchMovement();
    processCenterCameraHitTest();
    notifyARPointCloud();
    notifyCameraTransform(camera);
    processGazeEvent(ViroCardBoard::InputSource::Controller);
}

void VROInputControllerAR::onRotateStart(VROVector3f touchPos) {
    _isRotateOngoing = true;
    _latestRotation = 0; // reset latestRotation!
    VROVector3f rayFromCamera = calculateCameraRay(touchPos);
    VROInputControllerBase::updateHitNode(_latestCamera, _latestCamera.getPosition(), rayFromCamera);
    VROInputControllerBase::onRotate(ViroCardBoard::InputSource::Controller, 0, VROEventDelegate::RotateState::RotateStart);
}

void VROInputControllerAR::onRotate(float rotationRadians) {
    _latestRotation = rotationRadians;
}

void VROInputControllerAR::onRotateEnd() {
    _isRotateOngoing = false;
    VROInputControllerBase::onRotate(ViroCardBoard::InputSource::Controller, _latestRotation, VROEventDelegate::RotateState::RotateEnd);
}

void VROInputControllerAR::onPinchStart(VROVector3f touchPos) {
    _isPinchOngoing = true;
    _latestScale = 1.0; // reset latestScale!
    VROVector3f rayFromCamera = calculateCameraRay(touchPos);
    VROInputControllerBase::updateHitNode(_latestCamera, _latestCamera.getPosition(), rayFromCamera);
    VROInputControllerBase::onPinch(ViroCardBoard::InputSource::Controller, 1.0, VROEventDelegate::PinchState::PinchStart);
}

void VROInputControllerAR::onPinchScale(float scale) {
    _latestScale = scale;
}

void VROInputControllerAR::onPinchEnd() {
    _isPinchOngoing = false;
    VROInputControllerBase::onPinch(ViroCardBoard::InputSource::Controller, _latestScale, VROEventDelegate::PinchState::PinchEnd);
}

void VROInputControllerAR::onScreenTouchDown(VROVector3f touchPos) {
    _latestTouchPos = touchPos;
    _isTouchOngoing = true;
    VROVector3f rayFromCamera = calculateCameraRay(_latestTouchPos);
    VROInputControllerBase::updateHitNode(_latestCamera, _latestCamera.getPosition(), rayFromCamera);
    VROInputControllerBase::onButtonEvent(ViroCardBoard::ViewerButton, VROEventDelegate::ClickState::ClickDown);
}

void VROInputControllerAR::onScreenTouchMove(VROVector3f touchPos) {
    _latestTouchPos = touchPos;
}

void VROInputControllerAR::onScreenTouchUp(VROVector3f touchPos) {
    _latestTouchPos = touchPos;
    _isTouchOngoing = false;
    VROVector3f rayFromCamera = calculateCameraRay(_latestTouchPos);
    VROInputControllerBase::updateHitNode(_latestCamera, _latestCamera.getPosition(), rayFromCamera);
    VROInputControllerBase::onButtonEvent(ViroCardBoard::ViewerButton, VROEventDelegate::ClickState::ClickUp);
    
    // on touch up, we should invoke processDragging once more in case
    if (_lastDraggedNode) {
        // in AR, source is always the controller.
        processDragging(ViroCardBoard::InputSource::Controller, true);
    }
}

void VROInputControllerAR::processDragging(int source) {
    processDragging(source, false);
}

void VROInputControllerAR::processDragging(int source, bool alwaysRun) {
        std::shared_ptr<VRONode> draggedNode = _lastDraggedNode->_draggedNode;

    if (draggedNode->getDragType() != VRODragType::FixedToWorld) {
        VROInputControllerBase::processDragging(source);
        return;
    }
    
    std::shared_ptr<VROARSession> session = _weakSession.lock();
    if (session) {
        std::unique_ptr<VROARFrame> &frame = session->getLastFrame();
        std::vector<std::shared_ptr<VROARHitTestResult>> results = frame->hitTest(_latestTouchPos.x,
                                                                 _latestTouchPos.y,
                                                                 { VROARHitTestResultType::ExistingPlaneUsingExtent,
                                                                     VROARHitTestResultType::ExistingPlane,
                                                                     VROARHitTestResultType::EstimatedHorizontalPlane,
                                                                     VROARHitTestResultType::FeaturePoint });
        
        // If there are no AR results, then simply fallback to the base input controller's dragging logic.
        if (results.size() == 0) {
            VROInputControllerBase::processDragging(source);
            return;
        }

        // only process AR drag if we have a session and we've waited long enough since the last time we processed drag OR
        // if alwaysRun is true.
        if ((VROTimeCurrentMillis() - _lastProcessDragTimeMillis > kARProcessDragInterval) || alwaysRun) {

            VROVector3f position = getNextDragPosition(results);
            
            // TODO: since we're animating position... the position passed back below won't necessarily
            // reflect its real position.
            /*
             * To avoid spamming the JNI / JS bridge, throttle the notification
             * of onDrag delegates to a certain degree of accuracy.
             */
            float distance = position.distance(_lastDraggedNodePosition);
            if (distance < ON_DRAG_DISTANCE_THRESHOLD) {
                return;
            }

            // if we're already animating, then "cancel" it at the current position vs terminating
            // which causes the object to "jump" to the end.
            if (draggedNode->isAnimatingDrag() && draggedNode->getDragAnimation()) {
                VROTransaction::cancel(draggedNode->getDragAnimation());
                draggedNode->setIsAnimatingDrag(false);
            }
            
            // create new transaction to the new location
            VROTransaction::begin();
            VROTransaction::setAnimationDuration(.1);
            draggedNode->setWorldTransform(position, _lastDraggedNode->_originalDraggedNodeRotation, true);

            std::weak_ptr<VRONode> weakNode = draggedNode;
            VROTransaction::setFinishCallback([weakNode](bool terminate) {
                std::shared_ptr<VRONode> strongNode = weakNode.lock();
                if (strongNode) {
                    strongNode->setIsAnimatingDrag(false);
                }
            });
            VROTransaction::commit();
            
            // Update last known dragged position, distance to controller and notify delegates
            _lastDraggedNodePosition = position;
            _lastDraggedNode->_draggedDistanceFromController = position.distanceAccurate(_latestCamera.getPosition());
            
            draggedNode->getEventDelegate()->onDrag(source, draggedNode, position);
            for (std::shared_ptr<VROEventDelegate> delegate : _delegates) {
                delegate->onDrag(source, draggedNode, position);
            }
        }
    }
}

/*
 Things we're doing to select the next drag position (comments in the code below are more descriptive):
 - picking plane with extent if possible
 - looking at all feature points and filtering them (look in code to see what else we do)
 - finally, if all else fails, just move the object while maintaining the old distance.

 Things we aren't doing but could:
 - ignoring ExistingPlaneUsingExtent because it causes us to miss smaller objects (think
   trashcans/chairs) when a plane extends under the objects.
 - Smoothing the position - this might not be ideal in certain cases where we're certain of
   the position because if you drag an object from the table to the floor, there's no intermediary position.
 - Parse the point cloud ourselves to more directly influence the estimates.
 - "Shotgun" the area with hit tests to find the best position.

 Note: This function returns the position in WORLD coordinates, NOT dragged-node coordinates
 */
VROVector3f VROInputControllerAR::getNextDragPosition(std::vector<std::shared_ptr<VROARHitTestResult>> results) {
    VROVector3f cameraPos = _latestCamera.getPosition();
    
    // first, bucket the points, if we find an ExistingPlaneUsingExtent, then just return that (highest confidence)
    std::vector<std::shared_ptr<VROARHitTestResult>> featurePoints;
    std::shared_ptr<VROARHitTestResult> existingPlaneWithoutExtent = nullptr;
    for (std::shared_ptr<VROARHitTestResult> result : results) {
        switch (result->getType()) {
            case VROARHitTestResultType::ExistingPlaneUsingExtent:
                if (isDistanceWithinBounds(cameraPos, result->getWorldTransform().extractTranslation())) {
                    return result->getWorldTransform().extractTranslation();
                }
                break;
            case VROARHitTestResultType::ExistingPlane:
                existingPlaneWithoutExtent = result;
                break;
            case VROARHitTestResultType::FeaturePoint:
                featurePoints.push_back(result);
                break;
            default:
                // EstimatedPlane is not trusted enough to use as a drag position, because
                // there's too much volatility in their creation
                break;
        }
    }
    
    // Handle feature points. The most obnoxious thing we need to handle is the fact that ARKit likes to return
    // points REALLY close to the user or sometimes very far when it's unsure of things. This causes dragged
    // objects to ping-pong really close to really far from the user. We get around this by:
    // - preferring positions closest to the previous position
    // - having a min/max distance
    // - ensuring the point is in front of the user (sometimes it's actually behind the user ><)
    // - accepting new positions if they're far enough away, if they're moving away or if they're
    //   moving close to the user, but not by a large amount.
    if (featurePoints.size() > 0) {
        // Sort them by distance from the last dragged point.
        std::sort(featurePoints.begin(), featurePoints.end(), [this](std::shared_ptr<VROARHitTestResult> &a, std::shared_ptr<VROARHitTestResult> &b) {
            VROVector3f posA = a->getWorldTransform().extractTranslation();
            VROVector3f posB = b->getWorldTransform().extractTranslation();
            float distALast = posA.distance(this->_lastDraggedNodePosition);
            float distBLast = posB.distance(this->_lastDraggedNodePosition);
            return distALast < distBLast;
        });

        for (std::shared_ptr<VROARHitTestResult> &featurePoint : featurePoints) {
            VROVector3f featurePointPos = featurePoint->getWorldTransform().extractTranslation();
            VROVector3f ray = featurePointPos - _latestCamera.getPosition();
            // ensure the position is within bounds and is foward wrt the camera forward
            if (isDistanceWithinBounds(cameraPos, featurePointPos) && _latestCamera.getForward().dot(ray) > 0) {
                float candDistance = _latestCamera.getPosition().distance(featurePointPos);
                float distanceDiff = fabs(_lastDraggedNode->_draggedDistanceFromController - candDistance);
                if (candDistance > 2 || distanceDiff > _lastDraggedNode->_draggedDistanceFromController
                    || distanceDiff / _lastDraggedNode->_draggedDistanceFromController < .33) {
                    return featurePointPos;
                }
            }
        }
    }

    // TODO: remove this once we're more confident this doesn't help.
    // if we don't trust/have any feature points, then use the existingPlaneWithoutExtent.
//    if (existingPlaneWithoutExtent) {
//        VROVector3f planePos = existingPlaneWithoutExtent->getWorldTransform().extractTranslation();
//        if (isDistanceWithinBounds(cameraPos, planePos)) {
//            return planePos;
//        }
//    }

    // base case is to simply take the last dragged distance and keep the object that distance away from the controller.
    float distance = _lastDraggedNode->_draggedDistanceFromController;
    distance = fmin(distance, kARMaxDragDistance);
    distance = fmax(distance, kARMinDragDistance);
    VROVector3f touchForward = (results[0]->getWorldTransform().extractTranslation() - cameraPos).normalize();
    
    // sometimes the touch ray is calculated "behind" the camera forward, so just flip it.
    float projection = _latestCamera.getForward().dot(touchForward);
    if (projection < 0) {
        touchForward = touchForward * -1;
    }
    return cameraPos + (touchForward * distance);
}

bool VROInputControllerAR::isDistanceWithinBounds(VROVector3f point1, VROVector3f point2) {
    float distance = point1.distance(point2);
    return distance > kARMinDragDistance && distance < kARMaxDragDistance;
}

std::string VROInputControllerAR::getHeadset() {
    return std::string("mobile");
}

std::string VROInputControllerAR::getController() {
    return std::string("screen");
}

void VROInputControllerAR::processCenterCameraHitTest() {
    std::shared_ptr<VROARSession> session = _weakSession.lock();
    if (session && session->isReady()) {
        std::unique_ptr<VROARFrame> &frame = session->getLastFrame();
        std::shared_ptr<VROEventDelegate> delegate = _scene->getRootNode()->getEventDelegate();

        std::vector<std::shared_ptr<VROARHitTestResult>> results;
        if(delegate && frame && delegate->isEventEnabled(VROEventDelegate::EventAction::OnCameraARHitTest)) {
                std::shared_ptr<VROARCamera> camera = frame->getCamera();
                
            if(camera && (camera->getTrackingState() == VROARTrackingState::Unavailable)){
                // If delegate is enabled, send back empty results if tracking is not available yet.
                delegate->onCameraARHitTest(results);
                return;
            }
            
            VROQuaternion quaternion = camera->getRotation().extractRotation(VROVector3f(1.0f, 1.0f, 1.0f));

            bool bCameraChanged = false;
            VROVector3f currPosition = camera->getPosition();
            float distance = currPosition.distance(_cameraLastPosition);
            if(distance > 0.001f) {
                bCameraChanged = true;
            }

            //using formula : abs(q1.dot(q2)) > 1-EPS to determine equality of quaternions
            float quatDotProduct = fabs(quaternion.dotProduct(_cameraLastQuaternion));
            if(quatDotProduct <= (1- .000001)) {
                bCameraChanged = true;
            }

            if(bCameraChanged) {
                results = frame->hitTest(_viewportWidth/2.0f, _viewportHeight/2.0f,
                                     { VROARHitTestResultType::ExistingPlaneUsingExtent,
                                         VROARHitTestResultType::ExistingPlane,
                                         VROARHitTestResultType::EstimatedHorizontalPlane,
                                         VROARHitTestResultType::FeaturePoint });
                delegate->onCameraARHitTest(results);
            }
            _cameraLastQuaternion = quaternion;
            _cameraLastPosition = currPosition;
        }
    }
}

void VROInputControllerAR::notifyARPointCloud() {
    std::shared_ptr<VROARSession> session = _weakSession.lock();

    if (session && session->isReady()) {
        std::unique_ptr<VROARFrame> &frame = session->getLastFrame();
        std::shared_ptr<VROEventDelegate> delegate = _scene->getRootNode()->getEventDelegate();

        if (delegate && frame && delegate->isEventEnabled(VROEventDelegate::EventAction::OnARPointCloudUpdate)) {
            std::shared_ptr<VROARPointCloud> pointCloud = frame->getPointCloud();

            int pointCloudSize = (int)pointCloud->getPoints().size();
            // check if the last point cloud size is the same as the current one. That's a good
            // indication that the points didn't change (doing a simple pointer comparison
            // doesn't work on Android).
            if (pointCloudSize != _lastPointCloudSize && pointCloudSize > 0) {
                _lastPointCloudSize = pointCloudSize;
                delegate->onARPointCloudUpdate(pointCloud);
            }
        }
    }
}

void VROInputControllerAR::processTouchMovement() {
    if (_isTouchOngoing) {
        VROVector3f rayFromCamera = calculateCameraRay(_latestTouchPos);
        VROInputControllerBase::updateHitNode(_latestCamera, _latestCamera.getPosition(), rayFromCamera);
        VROInputControllerBase::onMove(ViroCardBoard::InputSource::Controller, _latestCamera.getPosition(), _latestCamera.getRotation(), rayFromCamera);
    } else {
        if (_isPinchOngoing) {
            VROInputControllerBase::onPinch(ViroCardBoard::InputSource::Controller, _latestScale, VROEventDelegate::PinchState::PinchMove);
        }
        if (_isRotateOngoing) {
             VROInputControllerBase::onRotate(ViroCardBoard::InputSource::Controller, _latestRotation, VROEventDelegate::RotateState::RotateMove);
        }
    }
}
 
VROVector3f VROInputControllerAR::calculateCameraRay(VROVector3f touchPos) {
    int viewportArr[4] = {0, 0, (int) _viewportWidth, (int) _viewportHeight};
    VROMatrix4f mvp = _projection.multiply(_view);

    // unproject the touchPos vector at z = 0 and z = 1
    VROVector3f resultNear;
    VROVector3f resultFar;
    
    VROProjector::unproject(VROVector3f(touchPos.x, touchPos.y, 0), mvp.getArray(), viewportArr, &resultNear);
    VROProjector::unproject(VROVector3f(touchPos.x, touchPos.y, 1), mvp.getArray(), viewportArr, &resultFar);

    // minus resultNear from resultFar to get the vector.
    return (resultFar - resultNear).normalize();
}


void VROInputControllerAR::processGazeEvent(int source) {
    if (_scene == nullptr){
        return;
    }

    std::shared_ptr<VROHitTestResult> result = _hitResult;

    _hitResult = std::make_shared<VROHitTestResult>(
            hitTest(_latestCamera, _latestCamera.getPosition(), _latestCamera.getForward(), true));

    VROInputControllerBase::processGazeEvent(ViroCardBoard::InputSource::Controller);

    // Restore the hit result.
    _hitResult = result;
}
