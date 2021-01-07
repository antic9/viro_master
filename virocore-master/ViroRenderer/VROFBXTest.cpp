//
//  VROFBXTest.cpp
//  ViroKit
//
//  Created by Raj Advani on 10/1/17.
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

#include "VROFBXTest.h"
#include "VROTestUtil.h"

VROFBXTest::VROFBXTest() :
    VRORendererTest(VRORendererTestType::FBX) {
    _angle = 0;
}

VROFBXTest::~VROFBXTest() {
    
}

void VROFBXTest::build(std::shared_ptr<VRORenderer> renderer,
                       std::shared_ptr<VROFrameSynchronizer> frameSynchronizer,
                       std::shared_ptr<VRODriver> driver) {

    _driver = driver;

    VROVector3f crPosition = { 0, -1, -4 };
    VROVector3f crScale = { .25, .25, .25 };
    
    VROFBXModel bumblebeecar("bumblebee_body", { 0, -2, -4 }, { 1, 1, 1 }, { 0, toRadians(70), 0 }, 1, "01");
    VROFBXModel cars("cr_cars", crPosition, crScale, { 0, 0, 0 }, 1, "01");
    VROFBXModel city("cr_city", crPosition, crScale, { 0, 0, 0 }, 1, "01");
    VROFBXModel floorplan("cr_floorplan", crPosition, crScale, { 0, 0, 0 }, 1, "01");
    VROFBXModel labels("cr_labels", crPosition, crScale, { 0, 0, 0 }, 1, "01");
    VROFBXModel logo("cr_logo", crPosition, crScale, { 0, 0, 0 }, 1, "01");
    VROFBXModel worm("worm", { 0, 0, -3 }, { .2, .2, .2 }, { 0, 0, 0 }, 1, "Take 001");
    VROFBXModel panther("object_bpanther_anim", { 0, -1.5, -8 }, { 2, 2, 2 }, { 0, 0, 0 }, 1, "01");
    VROFBXModel lamborghini("lamborghini_v2", { 0, -1.5, -6 }, { .015, .015, .015 }, { 0, 0, 0 }, 1, "02");
    VROFBXModel cylinder("cylinder_pbr", { 0, -1.85, -4 }, { 0.4, 0.4, 0.4 }, { 0, 0, 0 }, 1, "02_spin");
    VROFBXModel dragon("dragon", { 0, -1.5, -6 }, { 0.2, 0.2, 0.2 }, { 0, 0, 0 }, 1, "01");
    VROFBXModel pumpkin("pumpkin", { 0, -1.5, -3 }, { 1, 1, 1 }, { 0, 0, 0 }, 1, "02");
    
    _models.push_back({ cylinder });
    _models.push_back({ bumblebeecar });
    _models.push_back({ worm });
    _models.push_back({ cars, city, floorplan, labels, logo });
    _models.push_back({ panther });
    _models.push_back({ dragon });
    _models.push_back({ lamborghini });
    _models.push_back({ pumpkin });
    
    _sceneController = std::make_shared<VROARSceneController>();
    std::shared_ptr<VROScene> scene = _sceneController->getScene();
    
    std::shared_ptr<VROLight> light = std::make_shared<VROLight>(VROLightType::Spot);
    light->setColor({ 1.0, 1.0, 1.0 });
    light->setPosition( { 0, 3, -4 });
    light->setDirection( { 0, -1, 0 });
    light->setAttenuationStartDistance(25);
    light->setAttenuationEndDistance(50);
    light->setSpotInnerAngle(35);
    light->setSpotOuterAngle(60);
    light->setCastsShadow(false);
    light->setIntensity(10);
    
    std::shared_ptr<VROLight> ambient = std::make_shared<VROLight>(VROLightType::Ambient);
    ambient->setColor({ 1.0, 1.0, 1.0 });
    ambient->setIntensity(200);
    
    //std::shared_ptr<VROTexture> environment = VROTestUtil::loadRadianceHDRTexture("san_giuseppe_bridge_1k");
    std::shared_ptr<VROTexture> environment = VROTestUtil::loadRadianceHDRTexture("ibl_mans_outside");
    //std::shared_ptr<VROTexture> environment = VROTestUtil::loadRadianceHDRTexture("ibl_ridgecrest_road");
    //std::shared_ptr<VROTexture> environment = VROTestUtil::loadRadianceHDRTexture("ibl_wooden_door");
    
    std::shared_ptr<VROPortal> rootNode = scene->getRootNode();
    rootNode->setPosition({0, 0, 0});
    rootNode->addLight(light);
    rootNode->addLight(ambient);
    rootNode->setLightingEnvironment(environment);
    rootNode->setBackgroundSphere(environment);
    
    _fbxContainerNode = std::make_shared<VRONode>();
    rootNode->addChildNode(_fbxContainerNode);

    _fbxIndex = 0;
    rotateFBX();
    
    /*
     Shadow surface.
     */
    std::shared_ptr<VROSurface> surface = VROSurface::createSurface(80, 80);
    surface->setName("Surface");
    surface->getMaterials().front()->setLightingModel(VROLightingModel::Lambert);
    VROARShadow::apply(surface->getMaterials().front());
    
    std::shared_ptr<VRONode> surfaceNode = std::make_shared<VRONode>();
    surfaceNode->setGeometry(surface);
    surfaceNode->setRotationEuler({ -M_PI_2, 0, 0 });
    surfaceNode->setPosition({0, -6, -6});
    surfaceNode->setLightReceivingBitMask(1);
    rootNode->addChildNode(surfaceNode);
    
    std::shared_ptr<VRONodeCamera> camera = std::make_shared<VRONodeCamera>();
    camera->setRotationType(VROCameraRotationType::Orbit);
    camera->setOrbitFocalPoint({ 0, 0, -3});
    
    std::shared_ptr<VRONode> cameraNode = std::make_shared<VRONode>();
    cameraNode->setCamera(camera);
    rootNode->addChildNode(cameraNode);
    
    _pointOfView = cameraNode;
    
    _eventDelegate = std::make_shared<VROFBXEventDelegate>(this);
    _eventDelegate->setEnabledEvent(VROEventDelegate::EventAction::OnClick, true);
    rootNode->setEventDelegate(_eventDelegate);
}

void VROFBXTest::rotateFBX() {
    _fbxContainerNode->removeAllChildren();

    std::vector<VROFBXModel> models = _models[_fbxIndex];
    for (VROFBXModel &model : models) {
        std::shared_ptr<VRONode> fbxNode = VROTestUtil::loadFBXModel(model.name, model.position, model.scale, model.rotation,
                                                                     model.lightMask, model.animation, _driver, nullptr);
        _fbxContainerNode->addChildNode(fbxNode);
    }
    _fbxIndex = (_fbxIndex + 1) % _models.size();
}

void VROFBXEventDelegate::onClick(int source, std::shared_ptr<VRONode> node, ClickState clickState,
                                  std::vector<float> position) {
    if (clickState == ClickState::Clicked) {
        _test->rotateFBX();
    }
}
