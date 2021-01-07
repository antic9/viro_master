//
//  VROInputPresenterDaydream.h
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

#ifndef VROInputPresenterDaydream_H
#define VROInputPresenterDaydream_H

#include <memory>
#include <string>
#include <vector>
#include <VROReticle.h>
#include <VROBox.h>
#include <VROOBJLoader.h>
#include <VROPlatformUtil.h>
#include <VROModelIOUtil.h>
#include "VRONode.h"
#include "VRORenderContext.h"
#include "VROInputPresenter.h"
#include "VROInputType.h"
#include "VROMaterial.h"

class VROInputPresenterDaydream : public VROInputPresenter {
public:
    VROInputPresenterDaydream(std::shared_ptr<VRODriver> driver) {
        _rightHanded = true;
        // Initial values required for arm model
        _forwardVector = VROVector3f(0,0,-1);
        attachElbowNode();
        attachControllerNode(driver);
        attachLaserToController(driver);

        std::shared_ptr<VROTexture> reticleTexture
                = std::make_shared<VROTexture>(true,
                                               VROMipmapMode::Runtime,
                                               VROPlatformLoadImageFromAsset("dd_reticle_large.png", VROTextureInternalFormat::RGBA8));
        std::shared_ptr<VROReticle> reticle = std::make_shared<VROReticle>(reticleTexture);
        reticle->setPointerFixed(false);
        setReticle(reticle);
    }
    ~VROInputPresenterDaydream() {}

    void attachElbowNode() {
        _elbowNode = std::make_shared<VRONode>();
        _elbowNode->setSelectable(false);
        _elbowNode->setScale(VROVector3f(.2,  .2,  .2));
        _elbowNode->setPosition(_elbowNodePosition);
        getRootNode()->addChildNode(_elbowNode);
    }

    void attachControllerNode(std::shared_ptr<VRODriver> driver) {
        // Textures needed by the controller model representing different controller UI states
        _controllerIdleTexture = std::make_shared<VROTexture>(true,
                                                              VROMipmapMode::Runtime,
                                                              VROPlatformLoadImageFromAsset("ddcontroller_idle.jpg",
                                                                                            VROTextureInternalFormat::RGBA8));
        _controllerAppButtonPressedTexture = std::make_shared<VROTexture>(true,
                                                                          VROMipmapMode::Runtime,
                                                                          VROPlatformLoadImageFromAsset("ddcontroller_app.jpg",
                                                                                                        VROTextureInternalFormat::RGBA8));
        _controllerHomeButtonPressedTexture = std::make_shared<VROTexture>(true,
                                                                           VROMipmapMode::Runtime,
                                                                           VROPlatformLoadImageFromAsset("ddcontroller_system.jpg",
                                                                                                         VROTextureInternalFormat::RGBA8));
        _controllerTouchPadPressedTexture = std::make_shared<VROTexture>(true,
                                                                         VROMipmapMode::Runtime,
                                                                         VROPlatformLoadImageFromAsset("ddcontroller_touchpad.jpg",
                                                                                                       VROTextureInternalFormat::RGBA8));

        // Create Controller Obj Node
        std::string controllerObjAsset = VROPlatformCopyAssetToFile("ddcontroller.obj");
        _controllerNode = std::make_shared<VRONode>();
        VROOBJLoader::loadOBJFromResource(controllerObjAsset, VROResourceType::LocalFile, _controllerNode, driver, [this](std::shared_ptr<VRONode> node, bool success) {
            if (!success) {
                perr("ERROR when loading controller obj!");
                return;
            }

            // Set the lighting on this material to be constant
            const std::shared_ptr<VROMaterial> &material = node->getGeometry()->getMaterials().front();
            material->getDiffuse().setTexture(_controllerIdleTexture);
            material->setWritesToDepthBuffer(false);
            material->setReadsFromDepthBuffer(false);
            material->setReceivesShadows(false);
        });
        _controllerNode->setSelectable(false);

        // Set it at a predefined position height in relation to the elbow Node
        VROVector3f controllerPosition = _forwardVector * _foreArmLength;
        _controllerNode->setPosition(controllerPosition);

        // Attach it to the model - specifically to the Elbow Node
        _elbowNode->addChildNode(_controllerNode);
    }

    void attachLaserToController(std::shared_ptr<VRODriver> driver) {
        _laserTexture = std::make_shared<VROTexture>(true,
                                                     VROMipmapMode::Runtime,
                                                     VROPlatformLoadImageFromAsset("ddLaserTexture.jpg", VROTextureInternalFormat::RGBA8));
        // Create our laser obj
        std::string controllerObjAsset = VROPlatformCopyAssetToFile("ddlaser.obj");
        _pointerNode = std::make_shared<VRONode>();
        VROOBJLoader::loadOBJFromResource(controllerObjAsset, VROResourceType::LocalFile, _pointerNode, driver, [this](std::shared_ptr<VRONode> node, bool success) {
            if (!success) {
                perr("ERROR when loading controller obj!");
                return;
            }

            // Set the lighting on this material to be constant
            const std::shared_ptr<VROMaterial> &material = node->getGeometry()->getMaterials().front();
            material->setLightingModel(VROLightingModel::Constant);
            material->getDiffuse().setTexture(_laserTexture);
            material->setWritesToDepthBuffer(false);
            material->setReadsFromDepthBuffer(false);
            material->setReceivesShadows(false);
        });

        _pointerNode->setPosition(_controllerNode->getPosition());
        _pointerNode->setOpacity(0.6);
        _pointerNode->setSelectable(false);
        _elbowNode->addChildNode(_pointerNode);
    }

    void setTextureOnController(std::shared_ptr<VROTexture> texture){
        const std::shared_ptr<VROMaterial> &material = _controllerNode->getGeometry()->getMaterials().front();
        material->getDiffuse().clear();
        material->getDiffuse().setTexture(texture);
        material->setLightingModel(VROLightingModel::Constant);
    }

    void updateHandedness(bool isRightHanded){
        if (_rightHanded == isRightHanded){
            return;
        }

        float flip = isRightHanded ? 1 : -1;
        VROVector3f computedPosition = _elbowNodePosition + _elbowToCameraOffset;
        _elbowNode->setPosition(VROVector3f(computedPosition.x * flip,  computedPosition.y,  computedPosition.z));
        _rightHanded = isRightHanded;
    }

    void onClick(int source, std::shared_ptr<VRONode> node, ClickState clickState, std::vector<float> position) {
        VROInputPresenter::onClick(source, node, clickState, position);

        if (source ==ViroDayDream::InputSource::TouchPad && clickState == ClickState::ClickUp){
            getReticle()->trigger();
        }

        /*
         * Update Controller Buttons UI. It is assumed that only
         * one button is being clicked at a time.
         */
        if (clickState == ClickState::ClickUp){
            setTextureOnController(_controllerIdleTexture);
            return;
        }

        if (clickState == ClickState::ClickDown) {
            switch (source) {
                case ViroDayDream::TouchPad:
                    setTextureOnController(_controllerTouchPadPressedTexture);
                    break;
                case ViroDayDream::AppButton:
                    setTextureOnController(_controllerAppButtonPressedTexture);
                    break;
                case ViroDayDream::HomeButton:
                    setTextureOnController(_controllerHomeButtonPressedTexture);
                    break;
            }
        }
    }

    void onTouch(int source, std::shared_ptr<VRONode> node, TouchState touchState, float x, float y){
        VROInputPresenter::onTouch(source, node, touchState, x, y);
        if (touchState == TouchState::TouchUp){
            setTextureOnController(_controllerIdleTexture);
            return;
        }
    }

    void updateElbowOrientation(VROVector3f controllerRotation, VROVector3f cameraPosition) {
        _elbowToCameraOffset = cameraPosition;
        _elbowNode->setPosition(_elbowNodePosition + _elbowToCameraOffset);
        _elbowNode->setRotation(controllerRotation);
    }

    void onMove(int source, std::shared_ptr<VRONode> node, VROVector3f controllerRotation, VROVector3f controllerPosition, VROVector3f forwardVec){
        VROInputPresenter::onMove(source, node, controllerRotation, controllerPosition, forwardVec);
        _elbowNode->setRotation(controllerRotation);
    }

    virtual void onDrag(int source, std::shared_ptr<VRONode> node, VROVector3f newPosition) {
        VROInputPresenter::onDrag(source, node, newPosition);
    }

    void onGazeHit(int source, std::shared_ptr<VRONode> node, const VROHitTestResult &hit) {
        VROInputPresenter::onReticleGazeHit(hit);
     }

    std::shared_ptr<VRONode> getControllerPointerNode(){
        return _pointerNode;
    }

    std::shared_ptr<VRONode> getControllerNode(){
        return _controllerNode;
    }

private:
    const VROVector3f _elbowNodePosition = {0.29,  -0.78,  .18};
    VROVector3f _elbowToCameraOffset;

    bool _rightHanded;
    const float _foreArmLength = 2;
    VROVector3f _forwardVector;

    std::shared_ptr<VRONode> _pointerNode;
    std::shared_ptr<VRONode> _controllerNode;
    std::shared_ptr<VRONode> _elbowNode;

    // Textures representing different daydream states
    std::shared_ptr<VROTexture> _laserTexture;
    std::shared_ptr<VROTexture> _controllerIdleTexture;
    std::shared_ptr<VROTexture> _controllerAppButtonPressedTexture;
    std::shared_ptr<VROTexture> _controllerHomeButtonPressedTexture;
    std::shared_ptr<VROTexture> _controllerTouchPadPressedTexture;
};
#endif
