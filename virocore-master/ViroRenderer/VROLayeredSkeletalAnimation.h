//
//  VROLayeredSkeletalAnimation.h
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

#ifndef VROLayeredSkeletalAnimation_h
#define VROLayeredSkeletalAnimation_h

#include <memory>
#include <vector>
#include <map>
#include "VROMatrix4f.h"
#include "VROExecutableAnimation.h"

class VROShaderModifier;
class VROSkeleton;
class VROSkinner;
class VROSkeletalAnimation;
class VROAnimationChain;
class VROSkeletalAnimationLayer;

/*
 A single layer of a VROLayeredSkeletalAnimation. Each layer is comprised of a skeletal
 animation and the properties that define how it blends with the other skeletal animations.
 */
class VROSkeletalAnimationLayerInternal {
    
    friend class VRONode;
    friend class VROLayeredSkeletalAnimation;
    
public:
    VROSkeletalAnimationLayerInternal(std::string name, float defaultBoneWeight) :
        name(name), defaultBoneWeight(defaultBoneWeight) {
    }
    virtual ~VROSkeletalAnimationLayerInternal() {}
    
    float getBoneWeight(int boneIndex) {
        auto it = boneWeights.find(boneIndex);
        return (it == boneWeights.end()) ? defaultBoneWeight : it->second;
    }
    
private:
    std::string name;
    std::shared_ptr<VROSkeletalAnimation> animation;
    
    // Default weight for every bone influenced by this animation
    float defaultBoneWeight;
    
    // Overriden specific weights for bones
    std::map<int, float> boneWeights;
    
    // Derived keyframe data for the animation
    std::map<int, std::vector<float>> boneKeyTimes;
    std::map<int, std::vector<VROMatrix4f>> boneLocalTransforms;
    
    // Convert animation into keyframe data
    void buildKeyframes();
    
};

/*
 Drives multiple skeletal animations over the same skeleton, simultaneously.
 Each animation is given a bone-specific weight that determines its influence
 over each part of the skeleton.
 */
class VROLayeredSkeletalAnimation : public VROExecutableAnimation, public std::enable_shared_from_this<VROLayeredSkeletalAnimation> {
    
public:
    
    static std::shared_ptr<VROExecutableAnimation> createLayeredAnimation(std::vector<std::shared_ptr<VROSkeletalAnimationLayer>> layers);

    VROLayeredSkeletalAnimation(std::shared_ptr<VROSkinner> skinner,
                                std::vector<std::shared_ptr<VROSkeletalAnimationLayerInternal>> &layers,
                                float duration) :
            _skinner(skinner),
            _layers(layers),
            _duration(duration),
            _cached(false) {}

    virtual ~VROLayeredSkeletalAnimation() { }
    
    void setName(std::string name) {
        _name = name;
    }
    std::string getName() const {
        return _name;
    }
    
#pragma mark - Executable Animation API
    
    /*
     Produce a copy of this animation.
     */
    std::shared_ptr<VROExecutableAnimation> copy();
    
    /*
     Execute this animation. The onFinished() callback will be invoked when the
     animation is fully executed (when duration has transpired).
     
     Since this is a skeletal animation, the input node parameter is ignored. Skeletal
     animations are associated with a specific skeleton, and will animate all nodes
     connected to that skeleton.
     */
    void execute(std::shared_ptr<VRONode> node, std::function<void()> onFinished);
    void pause();
    void resume();
    void terminate(bool jumpToEnd);
    
    /*
     Blend the animation layers in advance so this starts without delay.
     */
    void preload();
    
    /*
     Set the duration of this layered skeletal animation, in seconds.
     */
    void setDuration(float durationSeconds);
    float getDuration() const;

    void setSpeed(float speed);

    std::string toString() const;
    
private:
    
    /*
     The name of this animation.
     */
    std::string _name;
    
    /*
     The skinner.
     */
    std::shared_ptr<VROSkinner> _skinner;
    
    /*
     The individual animations.
     */
    std::vector<std::shared_ptr<VROSkeletalAnimationLayerInternal>> _layers;
    
    /*
     The duration of this animation in seconds.
     */
    float _duration;
    
    /*
     Cache the blended bone times and values so if we re-run this animation these do not have
     to be recomputed. The _cached vector indicates which frames are already cached.
     */
    std::vector<bool> _cached;
    std::map<int, std::vector<float>> _boneKeyTimes;
    std::map<int, std::vector<VROMatrix4f>> _boneTransforms;
    
    /*
     If the animation is running, this is its associated transaction.
     */
    std::weak_ptr<VROTransaction> _transaction;
    
    /*
     Blending methods to create the unified animation.
     */
    void blendFrame(int f);
    static VROMatrix4f blendBoneTransform(const VROMatrix4f &previous, const VROMatrix4f &next, float weight);
    static VROMatrix4f blendBoneTransforms(int bone, const std::vector<std::pair<VROMatrix4f, float>> &transformsAndWeights);
    
    /*
     Recursively flatten out chains of chains.
     */
    static void flattenAnimationChain(std::shared_ptr<VROAnimationChain> chain, std::vector<std::shared_ptr<VROExecutableAnimation>> *animations);
    
};

#endif /* VROLayeredSkeletalAnimation_h */
