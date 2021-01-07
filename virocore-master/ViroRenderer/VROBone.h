//
//  VROBone.h
//  ViroRenderer
//
//  Created by Raj Advani on 5/10/17.
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

#ifndef VROBone_h
#define VROBone_h

#include <map>
#include "VROMatrix4f.h"
#include "VROAnimatable.h"

/*
 The transform of each bone can be either concatenated, local, or legacy. Skeletal
 animations are responsible for setting the transform. The VROSkinner is able to
 handle each of these transform types to drive an animation.
 
 Concatenated transforms move the bone (and in turn any associated mesh vertices)
 from bind position in bone local space, to the animated position in model space.
 
 Local transforms move the bone from the bind position in bone local space to the
 animated position in the bone local space of the *parent* bone.
 
 Legacy transforms (deprecated) move the bone from the bind position in bone local
 space to the animated position in bone local space.
 */
enum class VROBoneTransformType {
    Concatenated,
    Local,
    Legacy
};

/*
 VROBones are the control points for skeletal animation. They are animatable: 
 to drive a skeletal animation, invoke setTransform() for the bones that should
 be animated within an animation block. This will animate the bones, which in
 turn animates the VROSkeleton, which in turn (by way of VROSkinner objects) 
 animates any attached VROGeometry objects.
 
 The transform of each bone can be either concatnated, local, or legacy. Skeletal
 animations are responsible for setting the transform. The VROSkinner is able to
 handle each of these transform types to drive an animation. See VROBoneTransformType
 above for detail on each transform type, and VROSkinner.cpp for how Viro handles
 each transform type.

 Typically the transform is a concatenated, meaning each bone transform contains
 all transforms moving down the skeleton hierarchy: e.g., the transform for the
 finger bone is a concatenation of the upper arm transform, the lower arm transform,
 the hand transform, and finally the finger transform.
 
 Regardless of the transform type, the input coordinates to the bone transform
 (e.g. the coordinates that are multiplied by the bone transform) are assumed
 to be in bone local space, bind position. The bindTransform in each VROSkinner is
 responsible for transforming geometries into this space and position. See VROSkinner.h
 for more details.
 */
class VROBone : public VROAnimatable {
    
public:
    
    VROBone(int boneIndex, int parentIndex, std::string name,
            VROMatrix4f localTransform, VROMatrix4f bindTransform) :
        _index(boneIndex),
        _parentIndex(parentIndex),
        _name(name),
        _localTransform(localTransform),
        _bindTransform(bindTransform),
        _transformType(VROBoneTransformType::Legacy) {
    }
    virtual ~VROBone() {}
    
    /*
     Set the animation transform matrix and type for this bone. These properties
     are injected by skeletal animations as they are run.
     */
    void setTransform(VROMatrix4f transform, VROBoneTransformType type = VROBoneTransformType::Legacy);

    /*
     Returns the index of this bone within the skeleton.
     */
    int getIndex() const {
        return _index;
    }

    /*
     Get the index of the parent bone.
     */
    int getParentIndex() const {
        return _parentIndex;
    }
    
    /*
     Get the animation transform matrix and type for this bone.
     */
    VROMatrix4f getTransform() const {
        return _transform;
    }
    VROBoneTransformType getTransformType() const {
        return _transformType;
    }

    /*
     Get the (non-animated) local transform for this bone, which moves from the
     bone's local space in bind position to the parent bone's space in bind position.
     */
    VROMatrix4f getLocalTransform() const {
        return _localTransform;
    }

    /*
     Returns the binding transformation to use when moving from model space into
     bone local space that is configured in the "T-pose" bind position.
     */
    VROMatrix4f getBindTransform() const {
        return _bindTransform;
    }

    /*
     Returns the name associated with this bone, if any.
     */
    std::string getName() {
        return _name;
    }

    /*
     Sets a map of attachment transforms related to this bone.
     */
    void setAttachmentTransforms(std::map<std::string, VROMatrix4f> attachmentTransforms) {
        _attachmentTransforms = attachmentTransforms;
    }

    /*
     Returns a map of attachment transforms related to this bone.
     */
    std::map<std::string, VROMatrix4f> getAttachmentTransforms() {
        return _attachmentTransforms;
    }
    
private:
    
    /*
     The index of this node in the skeleton.
     */
    int _index;

    /*
     The index of this node's parent in the skeleton. This is an index into the parent
     VROSkeleton's _bones array.
     */
    int _parentIndex;
    
    /*
     Name of the bone (optional).
     */
    std::string _name;
    
    /*
     The transform to move from this bone, in bone local space, to the parent bone in its
     bone local space. In other words, this is the local transform when there is no animation
     (e.g. for the bind position). Put another way, if _transformType is Local and
     _transform == _localTransform, then this bone is not animated.
     */
    VROMatrix4f _localTransform;

    /*
     The transform from this bone's bind position in its bone local space, to its
     animated position in bone local space.
     */
    VROMatrix4f _transform;
    
    /*
     The type of transform installed in _transform. See VROBoneTransformType above for
     details. Defaults to Legacy.
     */
    VROBoneTransformType _transformType;

    /*
     The binding transformation to use when moving from model space into bone local
     space that is configured in the "T-pose" bind position.
     */
    VROMatrix4f _bindTransform;

    /*
     Attachment transforms relative to the bone in model space.
     */
    std::map<std::string, VROMatrix4f> _attachmentTransforms;
};

#endif /* VROBone_h */
