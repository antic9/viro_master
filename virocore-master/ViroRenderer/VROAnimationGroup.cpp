//
//  VROAnimationGroup.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 12/28/16.
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

#include "VROAnimationGroup.h"
#include "VROVector4f.h"
#include "VROGeometry.h"
#include "VRONode.h"
#include "VROMaterial.h"
#include "VROTransaction.h"
#include "VROStringUtil.h"
#include <sstream>

std::shared_ptr<VROAnimationGroup> VROAnimationGroup::parse(float durationSeconds, float delaySeconds, std::string functionName,
                                                            std::map<std::string, std::string> &propertyAnimations,
                                                            std::vector<std::shared_ptr<VROLazyMaterial>> materialAnimations) {
   
    VROTimingFunctionType timingFunction = parseTimingFunction(functionName);
    std::map<std::string, std::shared_ptr<VROPropertyAnimation>> pAnimations;
    for (auto kv : propertyAnimations) {
        std::string name = kv.first;
        pAnimations[name] = VROPropertyAnimation::parse(name, kv.second);
    }
    
    std::vector<std::shared_ptr<VROMaterialAnimation>> mAnimations;
    for (int i = 0; i < materialAnimations.size(); i++) {
        std::shared_ptr<VROMaterialAnimation> animation = std::make_shared<VROMaterialAnimation>(i, materialAnimations[i]);
        mAnimations.push_back(animation);
    }
    
    return std::make_shared<VROAnimationGroup>(durationSeconds, delaySeconds, timingFunction,
                                               pAnimations, mAnimations);
}

std::shared_ptr<VROExecutableAnimation> VROAnimationGroup::copy() {
    return std::make_shared<VROAnimationGroup>(_duration, _delay, _timingFunctionType,
                                               _propertyAnimations, _materialAnimations);
}

VROTimingFunctionType VROAnimationGroup::parseTimingFunction(std::string &name) {
    if (VROStringUtil::strcmpinsensitive(name, "Linear")) {
        return VROTimingFunctionType::Linear;
    }
    else if (VROStringUtil::strcmpinsensitive(name, "EaseIn")) {
        return VROTimingFunctionType::EaseIn;
    }
    else if (VROStringUtil::strcmpinsensitive(name, "EaseOut")) {
        return VROTimingFunctionType::EaseOut;
    }
    else if (VROStringUtil::strcmpinsensitive(name, "EaseInEaseOut")) {
        return VROTimingFunctionType::EaseInEaseOut;
    }
    else if (VROStringUtil::strcmpinsensitive(name, "Bounce")) {
        return VROTimingFunctionType::Bounce;
    }
    else if (VROStringUtil::strcmpinsensitive(name, "PowerDecel")) {
        return VROTimingFunctionType::PowerDecel;
    }
    
    //return default if nothing else matches
    return VROTimingFunctionType::Linear;
}

void VROAnimationGroup::execute(std::shared_ptr<VRONode> node,
                                std::function<void()> onFinished) {
    VROTransaction::begin();
    VROTransaction::setAnimationDelay(_delay);
    VROTransaction::setAnimationDuration(_duration);
    VROTransaction::setTimingFunction(_timingFunctionType);
    VROTransaction::setAnimationSpeed(_speed);
    VROTransaction::setAnimationTimeOffset(_timeOffset);
  
    animateMaterial(node);
    animatePosition(node);
    animateColor(node);
    animateOpacity(node);
    animateScale(node);
    animateRotation(node);

    std::weak_ptr<VROAnimationGroup> weakSelf = shared_from_this();
    VROTransaction::setFinishCallback([weakSelf, onFinished](bool terminate){
        std::shared_ptr<VROAnimationGroup> group = weakSelf.lock();
        if (group) {
            group->_transaction.reset();
        }
        onFinished();
    });
        
    _transaction = VROTransaction::commit();
}

void VROAnimationGroup::resume() {
    if (_transaction) {
        VROTransaction::resume(_transaction);
    }
}

void VROAnimationGroup::pause() {
    if (_transaction) {
        VROTransaction::pause(_transaction);
    }
}

void VROAnimationGroup::terminate(bool jumpToEnd) {
    if (_transaction) {
        VROTransaction::terminate(_transaction, jumpToEnd);
        _transaction.reset();
    }
}

void VROAnimationGroup::animateMaterial(std::shared_ptr<VRONode> &node) {
    if (_materialAnimations.empty()) {
        return;
    }
    if (!node->getGeometry()) {
        return;
    }
    
    for (std::shared_ptr<VROMaterialAnimation> &animation : _materialAnimations) {
        const std::vector<std::shared_ptr<VROMaterial>> &materials = node->getGeometry()->getMaterials();
        if (animation->getIndex() >= materials.size()) {
            continue;
        }
        
        std::shared_ptr<VROMaterial> materialStart = materials[animation->getIndex()];
        std::shared_ptr<VROMaterial> materialEnd   = animation->getMaterial();
        if (!materialStart || !materialEnd) {
            continue;
        }
        
        if (materialEnd->getDiffuse().getTextureType() != VROTextureType::None) {
            materialStart->getDiffuse().setTexture(materialEnd->getDiffuse().getTexture());
        }
        else {
            materialStart->getDiffuse().setColor(materialEnd->getDiffuse().getColor());
        }
        
        materialStart->setShininess(materialEnd->getShininess());
        materialStart->setFresnelExponent(materialEnd->getFresnelExponent());
        materialStart->setCullMode(materialEnd->getCullMode());
        materialStart->setLightingModel(materialEnd->getLightingModel());
        materialStart->setWritesToDepthBuffer(materialEnd->getWritesToDepthBuffer());
        materialStart->setReadsFromDepthBuffer(materialEnd->getReadsFromDepthBuffer());
    }
}

void VROAnimationGroup::animatePosition(std::shared_ptr<VRONode> &node) {
    auto posX_it = _propertyAnimations.find("positionX");
    auto posY_it = _propertyAnimations.find("positionY");
    auto posZ_it = _propertyAnimations.find("positionZ");

    float posX = node->getPosition().x;
    if (posX_it != _propertyAnimations.end()) {
        std::shared_ptr<VROPropertyAnimation> &a = posX_it->second;
        node->setPositionX(a->processOp(posX));
    }
    
    float posY = node->getPosition().y;
    if (posY_it != _propertyAnimations.end()) {
        std::shared_ptr<VROPropertyAnimation> &a = posY_it->second;
        node->setPositionY(a->processOp(posY));
    }
    
    float posZ = node->getPosition().z;
    if (posZ_it != _propertyAnimations.end()) {
        std::shared_ptr<VROPropertyAnimation> &a = posZ_it->second;
        node->setPositionZ(a->processOp(posZ));
    }
}

void VROAnimationGroup::animateScale(std::shared_ptr<VRONode> &node) {
    auto scaleX_it = _propertyAnimations.find("scaleX");
    auto scaleY_it = _propertyAnimations.find("scaleY");
    auto scaleZ_it = _propertyAnimations.find("scaleZ");
    
    float scaleX = node->getScale().x;
    if (scaleX_it != _propertyAnimations.end()) {
        std::shared_ptr<VROPropertyAnimation> &a = scaleX_it->second;
        node->setScaleX(a->processOp(scaleX));
    }
    
    float scaleY = node->getScale().y;
    if (scaleY_it != _propertyAnimations.end()) {
        std::shared_ptr<VROPropertyAnimation> &a = scaleY_it->second;
        node->setScaleY(a->processOp(scaleY));
    }
    
    float scaleZ = node->getScale().z;
    if (scaleZ_it != _propertyAnimations.end()) {
        std::shared_ptr<VROPropertyAnimation> &a = scaleZ_it->second;
        node->setScaleZ(a->processOp(scaleZ));
    }
}

void VROAnimationGroup::animateColor(std::shared_ptr<VRONode> &node) {
    auto color_it = _propertyAnimations.find("color");
    if (color_it != _propertyAnimations.end()) {
        std::shared_ptr<VROPropertyAnimation> &animation = color_it->second;
        
        int color = animation->getValue().valueInt;
        float a = ((color >> 24) & 0xFF) / 255.0;
        float r = ((color >> 16) & 0xFF) / 255.0;
        float g = ((color >> 8)  & 0xFF) / 255.0;
        float b =  (color & 0xFF) / 255.0;

        VROVector4f vecColor(r, g, b, a);
        if (node->getGeometry()) {
            std::shared_ptr<VROGeometry> geometry = node->getGeometry();
            
            for (const std::shared_ptr<VROMaterial> &material : geometry->getMaterials()) {
                material->getDiffuse().setColor(vecColor);
            }
        }
    }
}

void VROAnimationGroup::animateOpacity(std::shared_ptr<VRONode> &node) {
    auto opacity_it = _propertyAnimations.find("opacity");
    if (opacity_it != _propertyAnimations.end()) {
        std::shared_ptr<VROPropertyAnimation> &a = opacity_it->second;
        node->setOpacity(a->processOp(node->getOpacity()));
    }
}

void VROAnimationGroup::animateRotation(std::shared_ptr<VRONode> &node) {
    auto rotateX_it = _propertyAnimations.find("rotateX");
    auto rotateY_it = _propertyAnimations.find("rotateY");
    auto rotateZ_it = _propertyAnimations.find("rotateZ");
    
    VROVector3f rotation = node->getRotationEuler();
    
    float rotateX = rotation.x;
    if (rotateX_it != _propertyAnimations.end()) {
        std::shared_ptr<VROPropertyAnimation> &a = rotateX_it->second;
        node->setRotationEulerX(a->processOp(rotateX));
    }
    
    float rotateY = rotation.y;
    if (rotateY_it != _propertyAnimations.end()) {
        std::shared_ptr<VROPropertyAnimation> &a = rotateY_it->second;
        node->setRotationEulerY(a->processOp(rotateY));
    }
    
    float rotateZ = rotation.z;
    if (rotateZ_it != _propertyAnimations.end()) {
        std::shared_ptr<VROPropertyAnimation> &a = rotateZ_it->second;
        node->setRotationEulerZ(a->processOp(rotateZ));
    }
}

void VROAnimationGroup::setDuration(float durationSeconds) {
    _duration = durationSeconds;
}

float VROAnimationGroup::getDuration() const {
    return _duration;
}

void VROAnimationGroup::setSpeed(float speed) {
    _speed = speed;
    if (_transaction) {
        VROTransaction::setAnimationSpeed(_transaction, _speed);
    }
}

std::string VROAnimationGroup::toString() const {
    std::stringstream ss;
    ss << "[duration: " << _duration << ", delay: " << _delay;
    
    for (auto kv : _propertyAnimations) {
        ss << ", " << kv.first << ":" << kv.second->toString();
    }
    ss << "]";
    
    return ss.str();
}
