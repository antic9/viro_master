//
//  VROScene.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 10/19/15.
//  Copyright © 2015 Viro Media. All rights reserved.
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
#include <algorithm>
#include "VROScene.h"
#include "VRORenderContext.h"
#include "VRONode.h"
#include "VROPortal.h"
#include "VROGeometry.h"
#include "VROInputControllerBase.h"
#include "VROHitTestResult.h"
#include "VROLog.h"
#include "VROAudioPlayer.h"
#include "VROPencil.h"
#include "VROToneMappingRenderPass.h"
#include <stack>
#include <algorithm>

VROScene::VROScene() : VROThreadRestricted(VROThreadName::Renderer),
    _postProcessingEffectsUpdated(false),
    _toneMappingEnabled(true),
    _toneMappingMethod(VROToneMappingMethod::HableLuminanceOnly),
    _toneMappingExposure(kToneMappingDefaultExposure),
    _toneMappingWhitePoint(kToneMappingDefaultWhitePoint),
    _toneMappingUpdated(false) {
        
    _rootNode = std::make_shared<VROPortal>();
    _rootNode->setName("Root");
    _activePortal = _rootNode;
    _activePortal->setPassable(true);
    
    ALLOCATION_TRACKER_ADD(Scenes, 1);
}

VROScene::~VROScene() {
    ALLOCATION_TRACKER_SUB(Scenes, 1);
}

#pragma mark - Render Cycle

void VROScene::computeTransforms() {
    _rootNode->computeTransforms({}, {});
}

void VROScene::updateVisibility(const VRORenderContext &context) {
    _rootNode->updateVisibility(context);
}

void VROScene::applyConstraints(const VRORenderContext &context) {
    _rootNode->applyConstraints(context, {}, false);
}

void VROScene::computeIKRig(const VRORenderContext &context) {
    _rootNode->computeIKRig();
}

void VROScene::syncAtomicRenderProperties() {
    _rootNode->syncAppThreadProperties();
}

void VROScene::updateParticles(const VRORenderContext &context) {
    _rootNode->updateParticles(context);
}

void VROScene::updateSortKeys(std::shared_ptr<VRORenderMetadata> &metadata,
                              const VRORenderContext &context, std::shared_ptr<VRODriver> &driver) {
    passert_thread(__func__);
    
    if (kDebugSortOrder && context.getFrame() % kDebugSortOrderFrameFrequency == 0) {
        pinfo("Updating sort keys");
        VRONode::resetDebugSortIndex();
    }

    _lights.clear();
    _rootNode->collectLights(&_lights);

    VRORenderParameters renderParams;
    renderParams.lights = _lights;
    _rootNode->updateSortKeys(0, renderParams, metadata, context, driver);
    
    createPortalTree(context);
    _portals.walkTree([] (std::shared_ptr<VROPortal> portal) {
        portal->sortNodesBySortKeys();
    });
    
    _distanceOfFurthestObjectFromCamera = renderParams.furthestDistanceFromCamera;
}

#pragma mark - Portals

std::shared_ptr<VROPortal> VROScene::getRootNode() {
    return _rootNode;
}

void VROScene::setActivePortal(const std::shared_ptr<VROPortal> portal) {
    passert (hasNode(std::dynamic_pointer_cast<VRONode>(portal)));
    _activePortal = portal;
}

void VROScene::createPortalTree(const VRORenderContext &context) {
    _portals.children.clear();
    _portals.value.reset();
    _activePortal->traversePortals(context.getFrame(), 0, nullptr, &_portals);
    
    // Sort each recursion level by distance from camera, so that we render
    // sibling portals (portals on same recursion level) front to back
    sortSiblingPortals(_portals, context);
}

void VROScene::sortSiblingPortals(tree<std::shared_ptr<VROPortal>> &node, const VRORenderContext &context) {
    std::vector<tree<std::shared_ptr<VROPortal>>> &portals = node.children;
    std::sort(portals.begin(), portals.end(), [context](tree<std::shared_ptr<VROPortal>> &a, tree<std::shared_ptr<VROPortal>> &b) {
        passert (a.value->getRecursionLevel() == b.value->getRecursionLevel());
        return a.value->getWorldPosition().distance(context.getCamera().getPosition()) <
                b.value->getWorldPosition().distance(context.getCamera().getPosition());
    });
    
    for (tree<std::shared_ptr<VROPortal>> &child : portals) {
        sortSiblingPortals(child, context);
    }
}

bool VROScene::hasNode(std::shared_ptr<VRONode> node) const {
    return hasNode_helper(_rootNode, node);
}

bool VROScene::hasNode_helper(const std::shared_ptr<VRONode> &candidate, const std::shared_ptr<VRONode> &node) const {
    if (candidate == node) {
        return true;
    }
    else {
        for (std::shared_ptr<VRONode> &child : candidate->getChildNodes()) {
            if (hasNode_helper(child, node)) {
                return true;
            }
        }
        return false;
    }
}

const tree<std::shared_ptr<VROPortal>> VROScene::getPortalTree() const {
    return _portals;
}

std::vector<std::shared_ptr<VROGeometry>> VROScene::getBackgrounds() const {
    std::vector<std::shared_ptr<VROGeometry>> backgrounds;
    getBackgrounds(_rootNode, backgrounds);
    
    return backgrounds;
}

void VROScene::getBackgrounds(std::shared_ptr<VRONode> node, std::vector<std::shared_ptr<VROGeometry>> &backgrounds) const {
    if (node->getType() == VRONodeType::Portal) {
        std::shared_ptr<VROPortal> portal = std::dynamic_pointer_cast<VROPortal>(node);
        if (portal->getBackground() != nullptr) {
            backgrounds.push_back(portal->getBackground());
        }
    }
    
    for (std::shared_ptr<VRONode> &child : node->getChildNodes()) {
        getBackgrounds(child, backgrounds);
    }
}

#pragma mark - Input Controllers

void VROScene::detachInputController(std::shared_ptr<VROInputControllerBase> controller) {
    passert_thread(__func__);
    if (!_controllerPresenter){
        return;
    }

    std::shared_ptr<VRONode> node = _controllerPresenter->getRootNode();
    node->removeFromParentNode();
    
    controller->detachScene();
    _controllerPresenter = nullptr;
}

void VROScene::attachInputController(std::shared_ptr<VROInputControllerBase> controller) {
    passert_thread(__func__);

    std::shared_ptr<VROInputPresenter> presenter = controller->getPresenter();
    if (_controllerPresenter == presenter) {
        return;
    }

    std::shared_ptr<VRONode> node = presenter->getRootNode();
    _rootNode->addChildNode(node);
    _controllerPresenter = presenter;

    controller->attachScene(shared_from_this());
}

std::shared_ptr<VROInputPresenter> VROScene::getControllerPresenter() {
    return _controllerPresenter;
}

#pragma mark - Post-processing

void VROScene::setToneMappingEnabled(bool enabled) {
    _toneMappingEnabled = enabled;
    _toneMappingUpdated = true;
}

void VROScene::setToneMappingMethod(VROToneMappingMethod method) {
    _toneMappingMethod = method;
    _toneMappingUpdated = true;
}

void VROScene::setToneMappingExposure(float exposure) {
    _toneMappingExposure = exposure;
    _toneMappingUpdated = true;
}

void VROScene::setToneMappingWhitePoint(float whitePoint) {
    _toneMappingWhitePoint = whitePoint;
    _toneMappingUpdated = true;
}

void VROScene::setToneMappingUpdated(bool updated) {
    _toneMappingUpdated = updated;
}

void VROScene::setPostProcessingEffects(std::vector<std::string> effects) {
    _activePostProcessingEffects = effects;
    _postProcessingEffectsUpdated = true;
}

std::vector<std::string> VROScene::getPostProcessingEffects() const {
    return _activePostProcessingEffects;
}

void VROScene::setPostProcessingEffectsUpdated(bool updated) {
    _postProcessingEffectsUpdated = updated;
}

bool VROScene::isPostProcessingEffectsUpdated() const {
    return _postProcessingEffectsUpdated;
}

#pragma mark - Debug

void VROScene::drawBoundingBoxCorners(std::shared_ptr<VRONode> node,
                                      const VRORenderContext &context,
                                      std::shared_ptr<VRODriver> &driver) {
    std::shared_ptr<VROPencil> pencil = std::make_shared<VROPencil>();
    VROVector3f minPoint = VROVector3f(node->getBoundingBox().getMinX(), node->getBoundingBox().getMinY(), node->getBoundingBox().getMinZ());
    VROVector3f maxPoint = VROVector3f(node->getBoundingBox().getMaxX(), node->getBoundingBox().getMaxY(), node->getBoundingBox().getMaxZ());
    pencil->draw(minPoint, maxPoint);
    pencil->render(context, driver);
}

