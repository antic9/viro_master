//
//  VROPolylineTest.cpp
//  ViroKit
//
//  Created by Raj Advani on 10/16/17.
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

#include "VROPolylineTest.h"
#include "VROTestUtil.h"
#include "VROCompress.h"
#include "VROPolyline.h"
#include "VROToneMappingRenderPass.h"

VROPolylineTest::VROPolylineTest() :
    VRORendererTest(VRORendererTestType::Polyline) {
    
}

VROPolylineTest::~VROPolylineTest() {
    
}

void VROPolylineTest::build(std::shared_ptr<VRORenderer> renderer,
                            std::shared_ptr<VROFrameSynchronizer> frameSynchronizer,
                            std::shared_ptr<VRODriver> driver) {
    _sceneController = std::make_shared<VROSceneController>();
    std::shared_ptr<VROScene> scene = _sceneController->getScene();
    
    std::shared_ptr<VROPortal> rootNode = scene->getRootNode();
    
    std::shared_ptr<VROLight> light = std::make_shared<VROLight>(VROLightType::Spot);
    light->setColor({ 0.0, 0.0, 1.0 });
    light->setPosition( { 0, 0, 0 });
    light->setDirection( { 0, 0, -1.0 });
    light->setAttenuationStartDistance(25);
    light->setAttenuationEndDistance(50);
    light->setSpotInnerAngle(35);
    light->setSpotOuterAngle(60);
    light->setIntensity(1000);
    
    std::shared_ptr<VROLight> ambient = std::make_shared<VROLight>(VROLightType::Ambient);
    ambient->setColor({ 1.0, 1.0, 1.0 });
    ambient->setIntensity(200);
    
    std::shared_ptr<VROTexture> environment = VROTestUtil::loadRadianceHDRTexture("ibl_mans_outside");
    
    rootNode->addLight(light);
    rootNode->addLight(ambient);
    rootNode->setLightingEnvironment(environment);
    rootNode->setBackgroundSphere(environment);
    
    _polyline = std::make_shared<VROPolyline>();
    _polyline->setThickness(0.1);
    _polyline->getMaterials()[0]->setLightingModel(VROLightingModel::Lambert);
    _polyline->appendPoint({ -1, 0, 0 });
    _polyline->appendPoint({ 0, 0, .5 });
    _polyline->appendPoint({ 1, 0, 0 });
    
    std::shared_ptr<VRONode> polylineNode = std::make_shared<VRONode>();
    polylineNode->setIgnoreEventHandling(true);
    polylineNode->setGeometry(_polyline);
    polylineNode->setScale({ 0.5, 0.5, 0.5 });
    polylineNode->setPosition({ 0, 0, -1 });
    polylineNode->setRotationEuler({ M_PI / 6.0, 0, 0 });
    scene->getRootNode()->addChildNode(polylineNode);

    std::shared_ptr<VROBox> surface = VROBox::createBox(10, 10, 10);
    surface->getMaterials()[0]->setCullMode(VROCullMode::None);
    surface->getMaterials()[0]->getDiffuse().setColor({0.0, 0.2, 0.0, 1.0});
    
    std::shared_ptr<VRONode> surfaceNode = std::make_shared<VRONode>();
    surfaceNode->setPosition({ 0, 0, -2 });
    surfaceNode->setGeometry(surface);
    scene->getRootNode()->addChildNode(surfaceNode);
    
    _eventDelegate = std::make_shared<VROPolylineEventDelegate>(_polyline);
    _eventDelegate->setEnabledEvent(VROEventDelegate::EventAction::OnClick, true);
    _eventDelegate->setEnabledEvent(VROEventDelegate::EventAction::OnMove, true);
    _eventDelegate->setEnabledEvent(VROEventDelegate::EventAction::OnDrag, true);
    surfaceNode->setEventDelegate(_eventDelegate);
    
    std::shared_ptr<VRONodeCamera> camera = std::make_shared<VRONodeCamera>();
    camera->setRotationType(VROCameraRotationType::Orbit);
    camera->setOrbitFocalPoint({ 0, 0, -2 });
    
    std::shared_ptr<VRONode> cameraNode = std::make_shared<VRONode>();
    cameraNode->setCamera(camera);
    rootNode->addChildNode(cameraNode);
    
    _pointOfView = cameraNode;
}

void VROPolylineEventDelegate::onClick(int source, std::shared_ptr<VRONode> node,
                                       ClickState clickState, std::vector<float> position) {
    std::shared_ptr<VROPolyline> polyline = _polyline.lock();
    if (clickState == ClickState::Clicked && polyline && position.size() > 2) {
        VROVector3f pt = { position[0], position[1], position[2] };
        
        // Draw the next point interior to the box to avoid z-fighting
        polyline->appendPoint(pt.normalize().scale(pt.magnitude() * 0.98));
    }
}

void VROPolylineEventDelegate::onMove(int source, std::shared_ptr<VRONode> node,
                                      VROVector3f rotation, VROVector3f position, VROVector3f forwardVec) {
    
}

void VROPolylineEventDelegate::onDrag(int source, std::shared_ptr<VRONode> node,
                                      VROVector3f position) {
    
}

