//
//  VROLayeredSkeletalAnimation.cpp
//  ViroKit
//
//  Created by Raj Advani on 8/14/18.
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

#include "VROLayeredSkeletalAnimation.h"
#include "VROSkeletalAnimation.h"
#include "VROTransaction.h"
#include "VROLog.h"
#include "VROAnimationKeyFrameIndex.h"
#include "VROSkeleton.h"
#include "VROShaderModifier.h"
#include "VROBone.h"
#include "VROSkinner.h"
#include "VROAnimationChain.h"
#include "VROExecutableNodeAnimation.h"
#include "VROSkeletalAnimationLayer.h"
#include <sstream>
#include <map>

static const float kBlendEpsilon = 0.02;

void VROSkeletalAnimationLayerInternal::buildKeyframes() {
    // If the keyframes are already built, nothing to do here
    if (boneKeyTimes.size() > 0) {
        return;
    }
    for (const std::unique_ptr<VROSkeletalAnimationFrame> &frame : animation->getFrames()) {
        passert (frame->boneIndices.size() == frame->boneTransforms.size());
        
        for (int f = 0; f < frame->boneIndices.size(); f++) {
            int boneIndex = frame->boneIndices[f];
            boneKeyTimes[boneIndex].push_back(frame->time);
            boneLocalTransforms[boneIndex].push_back(frame->localBoneTransforms[f]);
        }
    }
}

void VROLayeredSkeletalAnimation::flattenAnimationChain(std::shared_ptr<VROAnimationChain> chain, std::vector<std::shared_ptr<VROExecutableAnimation>> *animations) {
    
    for (const std::shared_ptr<VROExecutableAnimation> &child : chain->getAnimations()) {
        std::shared_ptr<VROAnimationChain> childChain = std::dynamic_pointer_cast<VROAnimationChain>(child);
        if (childChain) {
            flattenAnimationChain(childChain, animations);
        } else {
            animations->push_back(child);
        }
    }
}

std::shared_ptr<VROExecutableAnimation> VROLayeredSkeletalAnimation::createLayeredAnimation(std::vector<std::shared_ptr<VROSkeletalAnimationLayer>> layers) {
    // Accumulate the finished animations (VROLayeredSkeletalAnimations and individual non-skeletal animations) here
    std::vector<std::shared_ptr<VROExecutableAnimation>> animations;
    // Accumulate weighted skeletal animations here. These are first sorted by the skinner (e.g. geo) each is using
    std::map<std::shared_ptr<VROSkinner>, std::vector<std::shared_ptr<VROSkeletalAnimationLayerInternal>>> skeletalLayers;
    float maxDuration = 0;
    
    for (const std::shared_ptr<VROSkeletalAnimationLayer> &layer : layers) {
        std::shared_ptr<VROExecutableAnimation> animation = layer->animation;
        
        // Recombine the skeletal animations found into new layered skeletal animations (we drop the input
        // layers)
        std::shared_ptr<VROAnimationChain> chain = std::dynamic_pointer_cast<VROAnimationChain>(animation);
        if (chain) {
            std::vector<std::shared_ptr<VROExecutableAnimation>> animationsInChain;
            flattenAnimationChain(chain, &animationsInChain);
            
            std::vector<std::shared_ptr<VROExecutableAnimation>> skeletalAnimationsInChain;
            
            for (const std::shared_ptr<VROExecutableAnimation> &child : animationsInChain) {
                std::shared_ptr<VROSkeletalAnimation> skeletal = nullptr;
                
                std::shared_ptr<VROExecutableNodeAnimation> nodeAnimation = std::dynamic_pointer_cast<VROExecutableNodeAnimation>(child);
                if (nodeAnimation) {
                    skeletal = std::dynamic_pointer_cast<VROSkeletalAnimation>(nodeAnimation->getInnerAnimation());
                } else {
                    skeletal = std::dynamic_pointer_cast<VROSkeletalAnimation>(child);
                }
                
                if (skeletal) {
                    // We have a skeletal animation, so it to our layer list. It will later be combined into
                    // a single VROLayeredSkeletalAnimation.
                    std::shared_ptr<VROSkinner> animationSkinner = skeletal->getSkinner();
                    std::vector<std::shared_ptr<VROSkeletalAnimationLayerInternal>> &layersForSkinner = skeletalLayers[animationSkinner];
                    
                    // Note we may have multiple skeletal animations that share the same name. In this case
                    // we give them all the same assigned weight. Same-name animations shouldn't together
                    // influence the same bones so this should be fine.
                    std::shared_ptr<VROSkeletalAnimationLayerInternal> layerInternal;
                    auto layerSkeletalIt = layer->_internal.find(animationSkinner);
                    if (layerSkeletalIt == layer->_internal.end()) {
                        layerInternal = std::make_shared<VROSkeletalAnimationLayerInternal>(layer->name, layer->defaultBoneWeight);
                        layerInternal->animation = skeletal;
                        layerInternal->boneWeights = layer->boneWeights;
                        
                        layer->_internal[animationSkinner] = layerInternal;
                    } else {
                        layerInternal = layerSkeletalIt->second;
                    }
                    
                    passert (layerInternal->boneWeights.size() > 0);
                    
                    layersForSkinner.push_back(layerInternal);
                    maxDuration = fmax(maxDuration, skeletal->getDuration());
                    
                } else {
                    // Probably a keyframe animation, just add it so it executes in parallel with the layered
                    // skeletal animations. Ensure it is not a chain (for sanity).
                    std::shared_ptr<VROAnimationChain> childChain = std::dynamic_pointer_cast<VROAnimationChain>(child);
                    passert (childChain == nullptr);
                    
                    animations.push_back(child);
                    maxDuration = fmax(maxDuration, child->getDuration());
                }
            }
        } else {
            std::shared_ptr<VROAnimationChain> chain = std::dynamic_pointer_cast<VROAnimationChain>(animation);
            passert (chain == nullptr);
            
            // It's an unknown animation (this shouldn't happen since VRONode::getAnimation() always returns chains.
            // Just add it.
            animations.push_back(animation);
        }
    }
    
    // Build the VROLayeredSkeletalAnimation and add it to the final animations list
    for (auto kv : skeletalLayers) {
        if (kv.second.size() > 0) {
            std::shared_ptr<VROLayeredSkeletalAnimation> layered = std::make_shared<VROLayeredSkeletalAnimation>(kv.first, kv.second, maxDuration);
            animations.push_back(layered);
        }
    }
    return std::make_shared<VROAnimationChain>(animations, VROAnimationChainExecution::Parallel);
}

std::shared_ptr<VROExecutableAnimation> VROLayeredSkeletalAnimation::copy() {
    std::vector<std::shared_ptr<VROSkeletalAnimationLayerInternal>> layers;
    for (std::shared_ptr<VROSkeletalAnimationLayerInternal> &origLayer : _layers) {
        std::shared_ptr<VROSkeletalAnimationLayerInternal> layer = std::make_shared<VROSkeletalAnimationLayerInternal>(origLayer->name, origLayer->defaultBoneWeight);
        layer->animation = origLayer->animation;
        layer->boneWeights = origLayer->boneWeights;
        layers.push_back(layer);
    }
    
    std::shared_ptr<VROLayeredSkeletalAnimation> animation = std::make_shared<VROLayeredSkeletalAnimation>(_skinner, layers, _duration);
    animation->setName(_name);
    animation->setTimeOffset(_timeOffset);
    animation->setSpeed(_speed);
    return animation;
}

void VROLayeredSkeletalAnimation::blendFrame(int f) {
    const std::vector<std::unique_ptr<VROSkeletalAnimationFrame>> &masterFrames = _layers[0]->animation->getFrames();
    std::vector<std::pair<VROMatrix4f, float>> boneTransformsToBlend;

    for (int b = 0; b < masterFrames[f]->boneIndices.size(); b++) {
        int boneIndex = masterFrames[f]->boneIndices[b];
        
        // Collect all layers that have a non-zero weight on this bone
        VROMatrix4f boneIdentityTransform = _skinner->getSkeleton()->getBone(boneIndex)->getLocalTransform();
        
        for (int i = 0; i < _layers.size(); i++) {
            std::shared_ptr<VROSkeletalAnimationLayerInternal> &layer = _layers[i];
            float weight = layer->getBoneWeight(boneIndex);
            if (weight > 0) {
                auto boneLocalTransform = layer->boneLocalTransforms.find(boneIndex);
                if (boneLocalTransform != layer->boneLocalTransforms.end() && f < boneLocalTransform->second.size()) {
                    boneTransformsToBlend.push_back({ boneLocalTransform->second[f], weight });
                }
            }
        }
        
        if (boneTransformsToBlend.size() == 0) {
            _boneTransforms[boneIndex][f] = boneIdentityTransform;
        } else if (boneTransformsToBlend.size() == 1) {
            _boneTransforms[boneIndex][f] = boneTransformsToBlend[0].first;
        } else {
            _boneTransforms[boneIndex][f] = blendBoneTransforms(boneIndex, boneTransformsToBlend);
        }
        boneTransformsToBlend.clear();
    }
}

void VROLayeredSkeletalAnimation::preload() {
    /*
     Build the keyframe animation data for each layer.
     */
    for (int i = 0; i < _layers.size(); i++) {
        _layers[i]->buildKeyframes();
    }
    
    if (_boneKeyTimes.size() == 0) {
        const std::vector<std::unique_ptr<VROSkeletalAnimationFrame>> &masterFrames = _layers[0]->animation->getFrames();
        for (int f = 0; f < masterFrames.size(); f++) {
            for (int b = 0; b < masterFrames[f]->boneIndices.size(); b++) {
                int boneIndex = masterFrames[f]->boneIndices[b];
                _boneKeyTimes[boneIndex].push_back(_layers[0]->boneKeyTimes[boneIndex][f]);
                _boneTransforms[boneIndex].push_back(VROMatrix4f::identity());
            }
            _cached.push_back(false);
        }
    }
}

void VROLayeredSkeletalAnimation::execute(std::shared_ptr<VRONode> node, std::function<void()> onFinished) {
    preload();
    const std::shared_ptr<VROSkeleton> &skeleton = _skinner->getSkeleton();
    
    /*
     Finally, begin the animation for each bone, all of which we be in a single transaction.
     */
    VROTransaction::begin();
    VROTransaction::setAnimationDuration(_duration);
    VROTransaction::setAnimationSpeed(_speed);
    VROTransaction::setAnimationTimeOffset(_timeOffset);
    VROTransaction::setTimingFunction(VROTimingFunctionType::Linear);
    
    std::string name = _name;
    for (auto kv : _boneKeyTimes) {
        int boneIndex = kv.first;
        std::vector<float> &keyTimes = kv.second;
        
        std::shared_ptr<VROBone> bone = skeleton->getBone(boneIndex);
        
        // TODO Use a a weak pointer for 'this'
        std::shared_ptr<VROAnimation> animation = std::make_shared<VROAnimationKeyframeIndex>([name, boneIndex, this](VROAnimatable *const animatable, int frame) {
            VROBone *bone = (VROBone *) animatable;
            
            if (!_cached[frame]) {
                blendFrame(frame);
                _cached[frame] = true;
            }
            bone->setTransform(_boneTransforms[boneIndex][frame], VROBoneTransformType::Local);
        }, keyTimes);
        
        bone->animate(animation);
    }
    
    std::weak_ptr<VROLayeredSkeletalAnimation> weakSelf = shared_from_this();
    VROTransaction::setFinishCallback([weakSelf, onFinished](bool terminate) {
        std::shared_ptr<VROLayeredSkeletalAnimation> skeletal = weakSelf.lock();
        if (skeletal) {
            skeletal->_transaction.reset();
        }
        onFinished();
    });
    
    std::shared_ptr<VROTransaction> transaction = VROTransaction::commit();
    transaction->holdExecutableAnimation(shared_from_this());
    
    _transaction = transaction;
}

VROMatrix4f VROLayeredSkeletalAnimation::blendBoneTransforms(int bone, const std::vector<std::pair<VROMatrix4f, float>> &transformsAndWeights) {
    passert (transformsAndWeights.size() >= 2);
    
    /*
     Reweight the transforms so they add to one.
     */
    std::vector<std::pair<VROMatrix4f, float>> reweighted;
    float totalWeight = 0;
    for (int i = 0; i < transformsAndWeights.size(); i++) {
        totalWeight += transformsAndWeights[i].second;
    }
    for (int i = 0; i < transformsAndWeights.size(); i++) {
        reweighted.push_back({ transformsAndWeights[i].first, transformsAndWeights[i].second / totalWeight });
    }
    
    /*
     Compute the blended transform.
     */
    VROMatrix4f blendedTransform = reweighted[0].first;
    float blendedWeight = reweighted[0].second;
    
    for (int i = 1; i < reweighted.size(); i++) {
        VROMatrix4f nextTransform = reweighted[i].first;
        float nextWeight = reweighted[i].second;
        
        // Skip tranforms that are below a given weight
        if (nextWeight < kBlendEpsilon) {
            continue;
        }
        
        // Reweight from the cumulative weight across all quaternions into a normalized
        // weight between just the last quaternion and the new one
        float totalWeight = blendedWeight + nextWeight;
        float twoValueBlendWeight = (nextWeight / totalWeight);
        
        // Optimization to avoid blending when values are extremely low or high
        if (twoValueBlendWeight > (1 - kBlendEpsilon)) {
            blendedTransform = nextTransform;
        } else if (twoValueBlendWeight > kBlendEpsilon) {
            blendedTransform = blendBoneTransform(blendedTransform, nextTransform, twoValueBlendWeight);
        } // else don't bother blending nextTransform into blendedTransform, its weight is too low to matter
        
        // Add the original weight of the latest transform to our total blend weight
        // (note this is distinct from the weight we just used to perform the blend --
        // that weight is just between the two quaternions, this weight is the weight
        // amongst *all* quaternions)
        blendedWeight += nextWeight;
    }
    return blendedTransform;
}

VROMatrix4f VROLayeredSkeletalAnimation::blendBoneTransform(const VROMatrix4f &previous, const VROMatrix4f &next, float weight) {
    // Merge the current animation with any previously stored animation for this bone
    VROVector3f   previousScale = previous.extractScale();
    VROQuaternion previousRotation = previous.extractRotation(previousScale);
    VROVector3f   previousTranslation = previous.extractTranslation();
    
    VROVector3f   currentScale = next.extractScale();
    VROQuaternion currentRotation = next.extractRotation(currentScale);
    VROVector3f   currentTranslation = next.extractTranslation();
    
    // Rotations are averaged by slerping
    VROQuaternion averageRotation = VROQuaternion::slerp(previousRotation, currentRotation, weight);
    VROMatrix4f averageTransform = averageRotation.getMatrix();
    
    // Translation is averaged by interpolation
    VROVector3f averageTranslation = previousTranslation.interpolate(currentTranslation, weight);
    averageTransform.translate(averageTranslation);
    return averageTransform;
}

void VROLayeredSkeletalAnimation::setSpeed(float speed) {
    _speed = speed;
    std::shared_ptr<VROTransaction> transaction = _transaction.lock();
    if (transaction) {
        VROTransaction::setAnimationSpeed(transaction, _speed);
    }
}

void VROLayeredSkeletalAnimation::pause() {
    std::shared_ptr<VROTransaction> transaction = _transaction.lock();
    if (transaction) {
        VROTransaction::pause(transaction);
    }
}

void VROLayeredSkeletalAnimation::resume() {
    std::shared_ptr<VROTransaction> transaction = _transaction.lock();
    if (transaction) {
        VROTransaction::resume(transaction);
    }
}

void VROLayeredSkeletalAnimation::terminate(bool jumpToEnd) {
    std::shared_ptr<VROTransaction> transaction = _transaction.lock();
    if (transaction) {
        VROTransaction::terminate(transaction, jumpToEnd);
        _transaction.reset();
    }
}

void VROLayeredSkeletalAnimation::setDuration(float durationSeconds) {
    _duration = durationSeconds;
}

float VROLayeredSkeletalAnimation::getDuration() const {
    return _duration;
}

std::string VROLayeredSkeletalAnimation::toString() const {
    std::stringstream ss;
    ss << "[layered-skeletal: " << _name << "]";
    return ss.str();
}
