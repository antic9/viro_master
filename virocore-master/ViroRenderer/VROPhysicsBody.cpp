//
//  VROPhysicsBody.m
//  ViroRenderer
//
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

#include "VROPhysicsBody.h"
#include "VROMatrix4f.h"
#include "VROGeometry.h"
#include <LinearMath/btTransform.h>
#include "VRONode.h"
#include "VROPhysicsMotionState.h"
#include "VROStringUtil.h"
#include <btBulletDynamicsCommon.h>

const std::string VROPhysicsBody::kDynamicTag = "Dynamic";
const std::string VROPhysicsBody::kKinematicTag = "Kinematic";
const std::string VROPhysicsBody::kStaticTag = "Static";

VROPhysicsBody::VROPhysicsBody(std::shared_ptr<VRONode> node, VROPhysicsBody::VROPhysicsBodyType type,
                               float mass, std::shared_ptr<VROPhysicsShape> shape) {
    if (type == VROPhysicsBody::VROPhysicsBodyType::Dynamic && mass == 0) {
        pwarn("Attempted to incorrectly set 0 mass for a dynamic body type! Defaulting to 1kg mass.");
        mass = 1;
    } else if (type != VROPhysicsBody::VROPhysicsBodyType::Dynamic && mass != 0) {
        pwarn("Attempted to incorrectly set mass for a static or kinematic body type! Defaulting to 0kg mass.");
        mass = 0;
    }

    // Default physics properties
    _enableSimulation = true;
    _useGravity = true;
    _w_node = node;
    _shape = shape;
    _type = type;
    _mass = mass;
    _inertia = VROVector3f(1,1,1);

    ++sPhysicsBodyIdCounter;
    _key = VROStringUtil::toString(sPhysicsBodyIdCounter);
    createBulletBody();

    // Schedule this physics body for an update in the computePhysics pass.
    _needsBulletUpdate = true;
}

VROPhysicsBody::~VROPhysicsBody() {
    releaseBulletBody();
}

void VROPhysicsBody::createBulletBody() {
    // Create the underlying Bullet Rigid body with a bullet shape if possible.
    // If no VROPhysicsShape is provided, one is inferred during a computePhysics pass.
    btVector3 inertia = btVector3(_inertia.x, _inertia.y, _inertia.z);
    btCollisionShape *collisionShape = _shape == nullptr ? nullptr : _shape->getBulletShape();
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(_mass , nullptr, collisionShape, inertia);

    _rigidBody = new btRigidBody(groundRigidBodyCI);
    _rigidBody->setUserPointer(this);

    // Set appropriate collision flags for the corresponding VROPhysicsBodyType
    if (_type == VROPhysicsBody::VROPhysicsBodyType::Kinematic) {
        _rigidBody->setCollisionFlags(_rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        _rigidBody->setActivationState(DISABLE_DEACTIVATION);
    } else if (_type == VROPhysicsBody::VROPhysicsBodyType::Static) {
        _rigidBody->setCollisionFlags(_rigidBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
    }
}

void VROPhysicsBody::releaseBulletBody() {
    _rigidBody->setUserPointer(nullptr);
    btMotionState *state = _rigidBody->getMotionState();
    if (state != nullptr) {
        delete state;
    }

    if (_rigidBody != nullptr){
        delete _rigidBody;
        _rigidBody = nullptr;
    }
}

btRigidBody* VROPhysicsBody::getBulletRigidBody() {
    return _rigidBody;
}

#pragma mark - RigidBody properties
std::string VROPhysicsBody::getKey() {
    return _key;
}

std::string VROPhysicsBody::getTag() {
    std::shared_ptr<VRONode> node = _w_node.lock();
    if (node) {
        return node->getTag();
    }
    return kDefaultNodeTag;
}

void VROPhysicsBody::setMass(float mass) {
    if (_type != VROPhysicsBody::VROPhysicsBodyType::Dynamic) {
        pwarn("Attempted to incorrectly set mass for a static or kinematic body type!");
        return;
    }
    _mass = mass;
    _rigidBody->setMassProps(mass, {_inertia.x, _inertia.y, _inertia.z});
}

void VROPhysicsBody::setInertia(VROVector3f inertia) {
    if (_type != VROPhysicsBody::VROPhysicsBodyType::Dynamic) {
        pwarn("Attempted to incorrectly set inertia for a static or kinematic body type!");
        return;
    }
    _inertia = inertia;
    _rigidBody->setMassProps(_mass, {inertia.x, inertia.y, inertia.z});
}

void VROPhysicsBody::setType(VROPhysicsBodyType type, float mass) {
    if (type == VROPhysicsBodyType::Kinematic && mass != 0){
        perror("Attempted to change body to a kinematic type with incorrect mass!");
        return;
    } else if (type != VROPhysicsBodyType::Kinematic && mass == 0){
        perror("Attempted to change body to a non-kinematic type with incorrect mass!");
        return;
    }

    if (type == VROPhysicsBody::VROPhysicsBodyType::Kinematic) {
        _rigidBody->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
        _rigidBody->setActivationState(DISABLE_DEACTIVATION);
    } else if (type == VROPhysicsBody::VROPhysicsBodyType::Static) {
        _rigidBody->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
        _rigidBody->setActivationState(ACTIVE_TAG);
    } else {
        _rigidBody->setActivationState(ACTIVE_TAG);
        _rigidBody->setCollisionFlags(0);
    }

    _type = type;
    setMass(mass);
    _needsBulletUpdate = true;
}

void VROPhysicsBody::setKinematicDrag(bool isDragging){
    if (isDragging) {
        _preservedDraggedMass = _mass;
        _preservedType = _type;
        setType(VROPhysicsBody::VROPhysicsBodyType::Kinematic, 0);
    } else {
        setType(_preservedType, _preservedDraggedMass);
    }

    // Refresh the motion state.
    _rigidBody->setMotionState(nullptr);
}

void VROPhysicsBody::setRestitution(float restitution) {
    _rigidBody->setRestitution(restitution);
}

void VROPhysicsBody::setFriction(float friction) {
    _rigidBody->setFriction(friction);
}

void VROPhysicsBody::setUseGravity(bool useGravity) {
    _useGravity = useGravity;
    if (useGravity){
        _rigidBody->activate(true);
    }
}

bool VROPhysicsBody::getUseGravity() {
    return _useGravity;
}

void VROPhysicsBody::setPhysicsShape(std::shared_ptr<VROPhysicsShape> shape) {
    if (_shape == shape) {
        return;
    }
    _shape = shape;

    // Bullet needs to refresh it's underlying object when changing its physics shape.
    _needsBulletUpdate = true;
}

void VROPhysicsBody::refreshBody() {
    _needsBulletUpdate = true;
}

void VROPhysicsBody::setIsSimulated(bool enabled) {
    if (_enableSimulation == enabled){
        return;
    }
    _enableSimulation = enabled;
    _needsBulletUpdate = true;
}

bool VROPhysicsBody::getIsSimulated() {
    return _enableSimulation;
}

void VROPhysicsBody::setPhysicsDelegate(std::shared_ptr<VROPhysicsBodyDelegate> delegate) {
    _w_physicsDelegate = delegate;
}

std::shared_ptr<VROPhysicsBodyDelegate> VROPhysicsBody::getPhysicsDelegate() {
    return _w_physicsDelegate.lock();
}

#pragma mark - Transfomation Updates
bool VROPhysicsBody::needsBulletUpdate() {
    return _needsBulletUpdate;
}

void VROPhysicsBody::updateBulletRigidBody() {
    if (!_needsBulletUpdate) {
        return;
    }

    std::shared_ptr<VRONode> node = _w_node.lock();
    if (node == nullptr) {
        pwarn("Mis-configured VROPhysicsBody is missing an attached node required for updating!");
        return;
    }

    // Update the rigid body to reflect the latest VROPhysicsShape.
    // If shape is not defined, we attempt to infer the shape from the node's geometry.
    if (_shape == nullptr && node->getGeometry()) {
        _shape = std::make_shared<VROPhysicsShape>(node, false);
    } else if (_shape && _shape->getIsGeneratedFromGeometry()) {
        _shape = std::make_shared<VROPhysicsShape>(node, _shape->getIsCompoundShape());
    } else if (_shape == nullptr) {
        pwarn("No collision shape detected for this rigidbody... defaulting to basic box shape.");
        std::vector<float> params = {1, 1, 1};
        _shape = std::make_shared<VROPhysicsShape>(VROPhysicsShape::VROShapeType::Box, params);
    }

    /*
     As Bullet's physicBody transform is placed at the center of mass of a physics object,
     it does not necessarily align with Viro's geometric transform that is placed at the position
     of a node's geometry. Thus, we will need to calculate geometricTransform-To-physicsTransform
     offset here to be stored in VROPhysicsMotionState.
     */
    btTransform physicsBodyTransformOffset = btTransform::getIdentity();
    if (_shape->getIsCompoundShape()) {
        btCompoundShape *compoundShape = (btCompoundShape *)_shape->getBulletShape();
        btVector3 principalInertia;

        btScalar* masses;
        int numShapes = compoundShape->getNumChildShapes();

        if (numShapes > 0){
            masses = new btScalar[numShapes];
            // Evenly distribute mass for this compound body
            for (int j=0; j<compoundShape->getNumChildShapes(); j++) {
                if (_mass > 0) {
                    masses[j] = _mass / compoundShape->getNumChildShapes();
                } else {
                    masses[j] = 1;
                }
            }
        } else {
            // Default to a single shaped mass if no shape.
            pwarn("Warning, attempted to create a compound shape with no sub shape! Ignoring update.");
            return;
        }
        
        // Recalculate the inertia and the center of mass offset of the compounded body
        compoundShape->calculatePrincipalAxisTransform(masses, physicsBodyTransformOffset, principalInertia);

        // Transform each sub-shape such that they are oriented in relation to the
        // calculated center of mass for this physics object. We then treat the center of
        // mass as the physicsBodyTransform offset.
        for (int i=0; i < compoundShape->getNumChildShapes(); i++) {
            btTransform newChildTransform = physicsBodyTransformOffset.inverse()*compoundShape->getChildTransform(i);
            compoundShape->updateChildTransform(i,newChildTransform);
        }

        // Update the inertia of the compound physics shape
        _rigidBody->setCollisionShape(compoundShape);
        _inertia = VROVector3f(principalInertia.x(), principalInertia.y(), principalInertia.z());
        _rigidBody->setMassProps(_mass, principalInertia);
        _rigidBody->updateInertiaTensor();
    } else {
        // If a physics shape was generated from geometric non-compound control, we then
        // place the physics shape at the center of the geometric bounding box associated
        // with this control and calculate the physicsBodyTransform offset. Note: the
        // only transformation that occurs here between the node and it's geometry
        // is a translation.
        if (_shape->getIsGeneratedFromGeometry()){
            // Grab the computed geometric transform contained within this node
            VROVector3f nodePosition = node->getWorldTransform().extractTranslation();
            VROVector3f geometryPosition = node->getBoundingBox().getCenter();
            VROVector3f geometryOffset = geometryPosition - nodePosition;
            VROMatrix4f geometryComputedTransform = node->getWorldTransform();
            geometryComputedTransform.translate(geometryOffset);

            // Calculate an offset transform between the geometry's compute transform,
            // and the node's compute transform. For example in VROText, the text geometry
            // is placed at a distance from the node's origin.
            VROMatrix4f computedTransformInvert = node->getWorldTransform().invert();
            VROMatrix4f offsetTransform = computedTransformInvert* geometryComputedTransform;

            // Finally save the re-computed transform as physicsBodyTransformOffset.
            VROVector3f pos = offsetTransform.extractTranslation();
            VROQuaternion rot = offsetTransform.extractRotation(offsetTransform.extractScale());

            VROVector3f nodeScale = node->getWorldTransform().extractScale();

            btTransform offsetTransformBullet = btTransform::getIdentity();
            offsetTransformBullet.setOrigin(btVector3({pos.x * nodeScale.x,
                                                       pos.y * nodeScale.y,
                                                       pos.z * nodeScale.z}));
            offsetTransformBullet.setRotation(btQuaternion({rot.X, rot.Y, rot.Z, rot.W}));
            physicsBodyTransformOffset = offsetTransformBullet;
        }
        _rigidBody->setCollisionShape(_shape->getBulletShape());

        // Update the inertia of the compound physics shape
        btVector3 principalInertia;
        _shape->getBulletShape()->calculateLocalInertia(_mass, principalInertia);
        _inertia = VROVector3f(principalInertia.x(), principalInertia.y(), principalInertia.z());
        _rigidBody->setMassProps(_mass, principalInertia);
        _rigidBody->updateInertiaTensor();
    }

    btMotionState *state = _rigidBody->getMotionState();
    if (state != nullptr) {
        delete state;
    }
    VROPhysicsMotionState *motionState = new VROPhysicsMotionState(shared_from_this(), physicsBodyTransformOffset);
    _rigidBody->setMotionState(motionState);

    // Update the rigid body to the latest world transform.
    btTransform transform;
    getWorldTransform(transform);
    _rigidBody->setWorldTransform(transform);

    // Set flag to false indicating that the modifications has applied to the bullet rigid body.
    _needsBulletUpdate = false;
}

void VROPhysicsBody::getWorldTransform(btTransform& centerOfMassWorldTrans) const {
    std::shared_ptr<VRONode> node = _w_node.lock();
    if (!node) {
        return;
    }

    VROVector3f pos = node->getWorldPosition();
    VROQuaternion rot = VROQuaternion(node->getWorldRotation());
    btTransform graphicsWorldTrans
            = btTransform(btQuaternion(rot.X, rot.Y, rot.Z, rot.W), btVector3(pos.x, pos.y, pos.z));

    VROPhysicsMotionState *state = (VROPhysicsMotionState *) _rigidBody->getMotionState();
    btTransform physicsTransformOffset = state->getPhysicsTransformOffset();
    centerOfMassWorldTrans = graphicsWorldTrans * physicsTransformOffset;
}

void VROPhysicsBody::setWorldTransform(const btTransform& centerOfMassWorldTrans) {
    std::shared_ptr<VRONode> node = _w_node.lock();
    if (!node) {
        return;
    }
    VROPhysicsMotionState *state = (VROPhysicsMotionState *) _rigidBody->getMotionState();
    btTransform physicsTransformOffset = state->getPhysicsTransformOffset();
    btTransform graphicsWorldTrans = centerOfMassWorldTrans * physicsTransformOffset.inverse();

    btQuaternion rot = graphicsWorldTrans.getRotation();
    btVector3 pos = graphicsWorldTrans.getOrigin();
    node->setWorldTransform({pos.getX() , pos.getY() , pos.getZ() },
                            VROQuaternion(rot.x(), rot.y(), rot.z(), rot.w()));
}

#pragma mark - Forces
void VROPhysicsBody::applyImpulse(VROVector3f impulse, VROVector3f offset) {
    _rigidBody->activate(true);
    _rigidBody->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z),
                            btVector3(offset.x, offset.y, offset.z));
}

void VROPhysicsBody::applyTorqueImpulse(VROVector3f impulse) {
    _rigidBody->activate(true);
    _rigidBody->applyTorqueImpulse(btVector3(impulse.x, impulse.y, impulse.z));
}

void VROPhysicsBody::applyForce(VROVector3f power, VROVector3f position) {
    BulletForce bulletForce;
    bulletForce.force = power;
    bulletForce.location = position;
    _forces.push_back(bulletForce);
}

void VROPhysicsBody::applyTorque(VROVector3f torque) {
    _torques.push_back(torque);
}

void VROPhysicsBody::clearForces() {
    _forces.clear();
    _torques.clear();
    _rigidBody->clearForces();
}

void VROPhysicsBody::updateBulletForces() {
    if (_forces.size() > 0 || _torques.size() > 0){
        _rigidBody->activate(true);
    }

    for (BulletForce bulletForce: _forces) {
        btVector3 force = btVector3(bulletForce.force.x, bulletForce.force.y, bulletForce.force.z);
        btVector3 atPosition
                = btVector3(bulletForce.location.x, bulletForce.location.y, bulletForce.location.z);
        _rigidBody->applyForce(force, atPosition);
    }

    for (VROVector3f torque: _torques) {
        _rigidBody->applyTorque(btVector3(torque.x, torque.y, torque.z));
    }
}

void VROPhysicsBody::setVelocity(VROVector3f velocity, bool isConstant) {
    if (isConstant){
        _constantVelocity = velocity;
    } else {
        _instantVelocity = velocity;
    }
}

void VROPhysicsBody::applyPresetVelocity() {
    if (_instantVelocity.magnitude() > 0) {
        _rigidBody->activate(true);
        _rigidBody->setLinearVelocity(btVector3(_instantVelocity.x,
                                                _instantVelocity.y,
                                                _instantVelocity.z));

        _instantVelocity = VROVector3f(0,0,0);
    } else if (_constantVelocity.magnitude() > 0) {
        _rigidBody->activate(true);
        _rigidBody->setLinearVelocity(btVector3(_constantVelocity.x,
                                                _constantVelocity.y,
                                                _constantVelocity.z));
    }
}
