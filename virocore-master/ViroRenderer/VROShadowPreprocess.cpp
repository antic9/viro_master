//
//  VROShadowPreprocess.cpp
//  ViroKit
//
//  Created by Raj Advani on 1/23/18.
//  Copyright © 2018 Viro Media. All rights reserved.
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

#include "VROShadowPreprocess.h"
#include "VROLight.h"
#include "VRODriver.h"
#include "VRORenderTarget.h"
#include "VROShadowMapRenderPass.h"
#include "VRORenderContext.h"
#include "VROScene.h"
#include "VROOpenGL.h" // For pglpush and pop

VROShadowPreprocess::VROShadowPreprocess(std::shared_ptr<VRODriver> driver) :
     _maxSupportedShadowMapSize(2048) {
         
    if (kDebugShadowMaps) {
        _shadowTarget = driver->newRenderTarget(VRORenderTargetType::DepthTexture, 1, kMaxShadowMaps, false, true);
    }
    else {
        _shadowTarget = driver->newRenderTarget(VRORenderTargetType::DepthTextureArray, 1, kMaxShadowMaps, false, true);
    }
}

void VROShadowPreprocess::execute(std::shared_ptr<VROScene> scene, VRORenderContext *context,
                                  std::shared_ptr<VRODriver> driver) {
    
    const std::vector<std::shared_ptr<VROLight>> &lights = scene->getLights();
    
    // Get the max requested shadow map size; use that for our render target
    int maxSize = 0;
    for (const std::shared_ptr<VROLight> &light : lights) {
        if (!light->getCastsShadow()) {
            continue;
        }
        maxSize = std::max(maxSize, light->getShadowMapSize());
    }
    
    if (maxSize == 0) {
        // No lights are casting a shadow
        return;
    }
    
    // Use the smallest of our max supported shadow map size and our max requested size
    int shadowMapSize = std::min(maxSize, _maxSupportedShadowMapSize);
    int minRequiredShadowMapSize = 128;
    
    // Set the shadow target's viewport. If we fail to create a shadow render target of
    // requested size, cut the size in half. If we continue to fail, then shadows map not
    // be supported by this device; in this case, return without rendering them.
    while (shadowMapSize >= minRequiredShadowMapSize) {
        _shadowTarget->setViewport({ 0, 0, shadowMapSize, shadowMapSize });
        if (_shadowTarget->hydrate()) {
            break;
        }
        else {
            shadowMapSize /= 2;
            _maxSupportedShadowMapSize = shadowMapSize;
        }
    }
    if (shadowMapSize < minRequiredShadowMapSize) {
        return;
    }
    
    std::map<std::shared_ptr<VROLight>, std::shared_ptr<VROShadowMapRenderPass>> activeShadowPasses;
    int i = 0;
    for (const std::shared_ptr<VROLight> &light : lights) {
        if (!light->getCastsShadow()) {
            continue;
        }
        passert (light->getType() != VROLightType::Ambient && light->getType() != VROLightType::Omni);
        
        std::shared_ptr<VROShadowMapRenderPass> shadowPass;
        
        // Get the shadow pass for this light if we already have one from the last frame;
        // otherwise, create a new one
        auto it = _shadowPasses.find(light);
        if (it == _shadowPasses.end()) {
            shadowPass = std::make_shared<VROShadowMapRenderPass>(light, driver);
        }
        else {
            shadowPass = it->second;
        }
        activeShadowPasses[light] = shadowPass;
        
        pglpush("Shadow Pass");
        if (!kDebugShadowMaps) {
            _shadowTarget->setTextureImageIndex(i, 0);
        }
        light->setShadowMapIndex(i);
        
        VRORenderPassInputOutput inputs;
        inputs.outputTarget = _shadowTarget;
        shadowPass->render(scene, nullptr, inputs, context, driver);
        
        driver->unbindShader();
        pglpop();
        
        ++i;
    }
    
    // If any shadow was rendered, set the shadow map in the context; otherwise
    // make it null
    if (i > 0) {
        context->setShadowMap(_shadowTarget->getTexture(0));
    }
    else {
        context->setShadowMap(nullptr);
    }
    
    // Shadow passes that weren't used this frame (e.g. are not in activeShadowPasses),
    // are removed
    _shadowPasses = activeShadowPasses;
}
