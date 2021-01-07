//
//  VROBodyIKController.cpp
//  ViroSample
//
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

#include "VROBodyIKController.h"
#include "VROInputControllerAR.h"
#include "VROBodyTracker.h"
#include "VROBodyPlayer.h"
#include "VROMatrix4f.h"
#include "VROSkeleton.h"
#include "VROBone.h"
#include "VROBox.h"
#include "VROIKRig.h"
#include "VROProjector.h"
#include "VROBillboardConstraint.h"
#include "VROTime.h"
#include "VROARFrame.h"

#if VRO_PLATFORM_IOS
#include "VRODriverOpenGLiOS.h"
#include "VROAnimBodyDataiOS.h"

static std::string pointLabels[16] = {
    "top\t\t\t", //0
    "neck\t\t", //1
    "R shoulder\t", //2
    "R elbow\t\t", //3
    "R wrist\t\t", //4
    "L shoulder\t", //5
    "L elbow\t\t", //6
    "L wrist\t\t", //7
    "R hip\t\t", //8
    "R knee\t\t", //9
    "R ankle\t\t", //10
    "L hip\t\t", //11
    "L knee\t\t", //12
    "L ankle\t\t", //13
    "L Thorax\t\t", //14
    "L Pelvis\t\t", //15
};

static UIColor *colors[16] = {
    [UIColor redColor],
    [UIColor greenColor],
    [UIColor blueColor],
    [UIColor cyanColor],
    [UIColor yellowColor],
    [UIColor magentaColor],
    [UIColor orangeColor],
    [UIColor purpleColor],
    [UIColor brownColor],
    [UIColor blackColor],
    [UIColor darkGrayColor],
    [UIColor lightGrayColor],
    [UIColor whiteColor],
    [UIColor grayColor],
    [UIColor redColor],
    [UIColor greenColor]
};
#endif

static const bool kCalculateBoneProportionality = false;
static const float kARHitTestWindowKernelPixel = 0.01;
static const float kAutomaticSizingRatio = 1;
static const bool kAutomaticResizing = true;

static const VROVector3f kInitialModelPos = VROVector3f(-10, -10, 10);
static const bool kUseTorsoClusteredDepth = false;
static const float kUsePresetDepthDistanceMeter = 1;

// Required joints needed for basic controller functionality (Scale / Root motion alignment)
static const VROBodyJointType kRequiredJoints[] = { VROBodyJointType::Neck,
                                                    VROBodyJointType::RightHip,
                                                    VROBodyJointType::LeftHip };
static const VROBodyJointType kArHitTestJoint = VROBodyJointType::Neck;
static const VROBodyJointType kIgnoredJoints[] = {  VROBodyJointType::Thorax,
                                                    VROBodyJointType::Pelvis};

// The hierarchy of ML joints as referred to by VROBodyIKController.
// Note: This is a different hierarchy than the one in VROIKRig.
static const std::map<VROBodyJointType, std::vector<VROBodyJointType>> kVROMLBodyTree = {
        {VROBodyJointType::Top,             {}},
        {VROBodyJointType::Neck,            {VROBodyJointType::Top,
                                                    VROBodyJointType::RightShoulder,
                                                    VROBodyJointType::LeftShoulder,
                                                    VROBodyJointType::RightHip,
                                                    VROBodyJointType::LeftHip}},
        {VROBodyJointType::RightShoulder,   {VROBodyJointType::RightElbow}},
        {VROBodyJointType::RightElbow,      {VROBodyJointType::RightWrist}},
        {VROBodyJointType::RightWrist,      {}},
        {VROBodyJointType::RightHip,        {VROBodyJointType::RightKnee}},
        {VROBodyJointType::RightKnee,       {VROBodyJointType::RightAnkle}},
        {VROBodyJointType::RightAnkle,      {}},
        {VROBodyJointType::LeftShoulder,    {VROBodyJointType::LeftElbow}},
        {VROBodyJointType::LeftElbow,       {VROBodyJointType::LeftWrist}},
        {VROBodyJointType::LeftWrist,       {}},
        {VROBodyJointType::LeftHip,         {VROBodyJointType::LeftKnee}},
        {VROBodyJointType::LeftKnee,        {VROBodyJointType::LeftAnkle}},
        {VROBodyJointType::LeftAnkle,       {}},
};

VROBodyIKController::VROBodyIKController(std::shared_ptr<VRORenderer> renderer,
                                                   std::shared_ptr<VRODriver> driver,
                                                   std::shared_ptr<VRONode> sceneRoot) {
    _currentTrackedState = VROBodyTrackedState::NotAvailable;
    _rig = nullptr;
    _skeleton = nullptr;
    _calibrating = false;
    _renderer = renderer;
    _mlRootToModelRoot = VROMatrix4f::identity();
    _bodyControllerRoot = std::make_shared<VRONode>();
    _isRecording = false;
    sceneRoot->addChildNode(_bodyControllerRoot);

    _displayDebugCubes = true;
    _shouldCalibrateRigWithResults = false;
    _hasValidProjectedPlane = false;
#if VRO_PLATFORM_IOS
    _view = (VROViewAR *) std::dynamic_pointer_cast<VRODriverOpenGLiOS>(driver)->getView();
#endif
}

VROBodyIKController::~VROBodyIKController() {
}

bool VROBodyIKController::bindModel(std::shared_ptr<VRONode> modelRootNode) {
    _rig = nullptr;
    _skeleton = nullptr;
    _keyToEffectorMap.clear();
    _mlJointForBoneIndex.clear();
    _bodyControllerRoot->removeAllChildren();

    // Ensure we clear out any IKRigs for previously set models.
    if (_modelRootNode != nullptr) {
        _modelRootNode->setIKRig(nullptr);
    }
    _modelRootNode = nullptr;

    // Bind and initialize a model to this controller
    std::vector<std::shared_ptr<VROSkinner>> skinners;
    modelRootNode->getSkinner(skinners, true);
    if (skinners.size() == 0) {
        perror("VROBodyIKController: Attempted to bind to a model without a properly configured skinner.");
        return false;
    }

    /*
     Iterate through all known VROBodyJointType and examine the skinner's
     skeleton for bones required for body tracking. For each found bone,
     create an effector in our IKRig, and as well as joint data caches
     needed by this controller.
     */
    std::shared_ptr<VROSkeleton> skeleton = skinners[0]->getSkeleton();
    std::map<VROBodyJointType, std::string>::const_iterator bonePair;
    for (bonePair = kVROBodyBoneTags.begin(); bonePair != kVROBodyBoneTags.end(); bonePair++) {
        VROBodyJointType boneType = bonePair->first;
        std::string expectedBoneName = bonePair->second;

        // Determine if the model has a bone name matching the desired ML joint.
        std::shared_ptr<VROBone> bone = skeleton->getBone(expectedBoneName);
        if (bone == nullptr) {
            // Bone for ML joint does not exist in skeleton, continue.
            pwarn("Unable to find bone %s for VROBodyIKController!", expectedBoneName.c_str());
            continue;
        }

        int boneIndex = bone->getIndex();
        _mlJointForBoneIndex[boneType] = boneIndex;
        _keyToEffectorMap[expectedBoneName] = boneIndex;
    }

    // If we have not found required bones, fail the binding of the model.
    for (auto requiredBone : kVROBodyBoneTags) {
        if (_mlJointForBoneIndex.find(requiredBone.first) == _mlJointForBoneIndex.end()) {
            perr("Attempted to bind 3D model with improperly configured bones to VROBodyTracker!");
            return false;
        }
    }

    // Else update our skeleton references if we have the proper ML bones in our 3D model.
    _skeleton = skinners[0]->getSkeleton();
    _modelRootNode = modelRootNode;
    _bodyControllerRoot->setScale(VROVector3f(1,1,1));


    // Initialize calibration event delegates
    if (_calibrationEventDelegate == nullptr) {
        _calibrationEventDelegate = std::make_shared<VROBodyIKControllerEventDelegate>(shared_from_this());
        _calibrationEventDelegate->setEnabledEvent(VROEventDelegate::EventAction::OnClick, false);
        _calibrationEventDelegate->setEnabledEvent(VROEventDelegate::EventAction::OnPinch, false);
    }

    // Set the model in it's original scale needed for determining ratios for automatic resizing.
    calculateSkeletonTorsoDistance();

    // Create debug effector nodes UI
    for (auto &debugBox : _debugBoxEffectors) {
        debugBox.second->removeFromParentNode();
    }
    if (_debugBoxRoot != nullptr) {
        _debugBoxRoot->removeFromParentNode();
    }
    _debugBoxEffectors.clear();

    for (auto bonePair : _mlJointForBoneIndex) {
        VROBodyJointType boneType = bonePair.first;
        std::string boneName = kVROBodyBoneTags.at(bonePair.first);
        VROVector3f pos = _skeleton->getCurrentBoneWorldTransform(boneName).extractTranslation();
        std::shared_ptr<VRONode> block = createDebugBoxUI(true, boneName);
        block->setOpacity(_displayDebugCubes ? 1.0 : 0.0);
        _bodyControllerRoot->addChildNode(block);
        block->setWorldTransform(pos, VROMatrix4f::identity());
        _debugBoxEffectors[boneType] = block;
    }

    // Create a debug root node UI
    _debugBoxRoot = createDebugBoxUI(false, "Root");
    _debugBoxRoot->setOpacity(_displayDebugCubes ? 1.0 : 0.0);
    _bodyControllerRoot->addChildNode(_debugBoxRoot);

    // Set the timeout of joints in milliseconds.
    _mlJointTimeoutMap.clear();
    _mlJointTimeoutMap[VROBodyJointType::Top] =            500;
    _mlJointTimeoutMap[VROBodyJointType::Neck] =           800;
    _mlJointTimeoutMap[VROBodyJointType::LeftShoulder] =   500;
    _mlJointTimeoutMap[VROBodyJointType::LeftElbow] =      500;
    _mlJointTimeoutMap[VROBodyJointType::LeftWrist] =      500;
    _mlJointTimeoutMap[VROBodyJointType::RightShoulder] =  500;
    _mlJointTimeoutMap[VROBodyJointType::RightElbow] =     500;
    _mlJointTimeoutMap[VROBodyJointType::RightWrist] =     500;
    _mlJointTimeoutMap[VROBodyJointType::LeftHip] =        500;
    _mlJointTimeoutMap[VROBodyJointType::LeftKnee] =       500;
    _mlJointTimeoutMap[VROBodyJointType::LeftAnkle] =      500;
    _mlJointTimeoutMap[VROBodyJointType::RightHip] =       500;
    _mlJointTimeoutMap[VROBodyJointType::RightKnee] =      500;
    _mlJointTimeoutMap[VROBodyJointType::RightAnkle] =     500;
    return true;
}

void VROBodyIKController::restoreTopBoneTransform() {
    std::shared_ptr<VRONode> parentNode = _modelRootNode->getParentNode();
    _modelRootNode->computeTransforms(parentNode->getWorldTransform(), parentNode->getWorldRotation());

    // Grab the first skinner to examine geometric transforms with.
    std::vector<std::shared_ptr<VROSkinner>> skinners;
    _modelRootNode->getSkinner(skinners, true);
    if (skinners.size() < 0) {
        return;
    }

    // Now determine if the geometry bind transforms for the top bone is the identity.
    std::shared_ptr<VROSkinner> skinner = skinners[0];
    const std::vector<VROMatrix4f> bindTrans = skinner->getBindTransforms();
    std::shared_ptr<VROBone> bone = _skeleton->getBone(kVROBodyBoneTags.at(VROBodyJointType::Top));
    int topIndex = bone->getIndex();
    if (!bindTrans.at(topIndex).isIdentity()) {
        return;
    }

    // If so, we'll need to restore the actual bone transform by 'unrolling' the skeleton.
    int boneIndex = bone->getIndex();
    int cIndex = boneIndex;
    std::vector<std::shared_ptr<VROBone>> bones;
    while (cIndex > 0) {
        bone = _skeleton->getBone(cIndex);
        bones.push_back(bone);
        cIndex = bone->getParentIndex();
    }

    VROMatrix4f computedBoneTransform = VROMatrix4f::identity();
    for (int i = (int) bones.size() - 1; i >= 0; i--) {
        computedBoneTransform = bones[i]->getLocalTransform() * computedBoneTransform;
    }

    // Move the resulting unrolled bone space transform into model space, and then world space.
    VROMatrix4f boneTransformModelSpace =  skinner->getInverseBindTransforms().at(topIndex).multiply(computedBoneTransform);
    VROMatrix4f skinnerNodeTrans = skinner->getSkinnerNode()->getWorldTransform();
    VROMatrix4f output = skinnerNodeTrans.multiply(boneTransformModelSpace);

    // Save the result back into the skeleton.
    _skeleton->setCurrentBoneWorldTransform(kVROBodyBoneTags.at(VROBodyJointType::Top), output, false);
}

void VROBodyIKController::startCalibration(bool manual) {
    if (_calibrating) {
        return;
    }

    if (_skeleton == nullptr) {
        pwarn("Unable to start calibration: Model has not yet been bounded to this controller!");
        return;
    }

    // Hook in event delegates for enabling the user to click-to-calibrate.
    if (manual) {
        _calibrationEventDelegate->setEnabledEvent(VROEventDelegate::EventAction::OnClick, true);
        _calibrationEventDelegate->setEnabledEvent(VROEventDelegate::EventAction::OnPinch, true);
        _modelRootNode->addConstraint(std::make_shared<VROBillboardConstraint>(VROBillboardAxis::Y));
        _preservedEventDelegate = _modelRootNode->getEventDelegate();
        _modelRootNode->setEventDelegate(_calibrationEventDelegate);
    }

    // Clear previously calibrated data.
    _mlBoneLengths.clear();
    _modelBoneLengths.clear();
    _calibrating = true;
    _hasValidProjectedPlane = false;
    _userTorsoHeight = 0;
    _projectedPlanePosition = kInitialModelPos;
    _projectedPlaneNormal = VROVector3f(0,0,0);

    // Reset the model and bones back to it's initial configuration
    std::shared_ptr<VRONode> parentNode = _modelRootNode->getParentNode();
    _modelRootNode->setScale(VROVector3f(1, 1, 1));
    _modelRootNode->setRotation(VROQuaternion());
    _modelRootNode->setPosition(kInitialModelPos);
    _modelRootNode->computeTransforms(parentNode->getWorldTransform(), parentNode->getWorldRotation());

    // reset the bones back to it's initial configuration
    std::shared_ptr<VROSkeleton> skeleton = _skeleton;
    for (int i = 0; i < skeleton->getNumBones(); i++) {
        std::shared_ptr<VROBone> bone = skeleton->getBone(i);
        bone->setTransform(VROMatrix4f::identity(), bone->getTransformType());
    }
}

void VROBodyIKController::finishCalibration(bool manual) {
    if (!_calibrating) {
        return;
    }

    // Disable any calibration event delegates when finishing calibration.
    if (manual) {
        _calibrationEventDelegate->setEnabledEvent(VROEventDelegate::EventAction::OnClick, false);
        _calibrationEventDelegate->setEnabledEvent(VROEventDelegate::EventAction::OnPinch, false);
        _modelRootNode->setEventDelegate(_preservedEventDelegate);
    }

    _hasValidProjectedPlane = true;
    _shouldCalibrateRigWithResults = true;
}

void VROBodyIKController::calibrateRigWithResults() {
    if (_skeleton == nullptr) {
        pwarn("Unable to finish calibration: Model has not yet been bounded to this controller!");
        return;
    }

    // Only calculate proportionality once calibration is done.
    //calibrateBoneProportionality();

    _modelRootNode->removeAllConstraints();
    //_rig = std::make_shared<VROIKRig>(_skeleton, _keyToEffectorMap);
    //_modelRootNode->setIKRig(_rig);

    // Start listening for new joint data.
    _calibrating = false;

    std::shared_ptr<VROBodyIKControllerDelegate> delegate = _delegate.lock();
    if (delegate) {
        delegate->onCalibrationFinished();
    }
    _shouldCalibrateRigWithResults = false;
}

void VROBodyIKController::calibrateBoneProportionality() {
    restoreTopBoneTransform();
    if (!kCalculateBoneProportionality) {
        return;
    }

    bool hasAllRequiredBones = true;
    for (auto bone : kVROMLBodyTree) {
        bool hasJoint = false;
        for (auto currentBone : _cachedTrackedJoints) {
            if (currentBone.first == bone.first) {
                hasJoint = true;
                break;
            }
        }
        
        // If we are missing a required joint, update flag and break out.
        if (!hasJoint) {
            hasAllRequiredBones = false;
            break;
        }
    }
    
    bool hasEffectorData = hasAllRequiredBones || _modelBoneLengths.size() != 0;
    if (!hasEffectorData) {
        pwarn("Currently tracking with limited joints... skipping bone calibration!");
        return;
    }

    // First calculate intermediate bone sizes with the latest MLJoint data
    if (_modelBoneLengths.size() == 0) {
        calculateKnownBoneSizes(VROBodyJointType::Neck);
        calculateInferredBoneSizes();
    }

    // Then, modify the model's skeleton with the calibrated lengths.
    scaleBoneTransform("mixamorig:Hips", "mixamorig:Spine2", {0.0f, 0.0f, 0.0f});
    scaleBoneTransform("mixamorig:Spine2", "Neck", {0.0f, 1.0f, 0.0f});
    scaleBoneTransform("mixamorig:Spine2", "RightShoulder", {0.0f, 0.0f, 0.0f});
    scaleBoneTransform("mixamorig:Spine2", "LeftShoulder", {0.0f, 0.0f, 0.0f});
    scaleBoneTransform("Neck", "Top", {0.0f, 0.0f, 0.0f});
    scaleBoneTransform("RightShoulder", "RightElbow", {1.0f, 0, 0});
    scaleBoneTransform("RightElbow", "RightWrist", {1.0f, 0, 0});
    scaleBoneTransform("LeftShoulder", "LeftElbow", {1.0f, 0, 0});
    scaleBoneTransform("LeftElbow", "LeftWrist", {1.0f, 0, 0});
    scaleBoneTransform("RightHip", "RightKnee", {0, 1.0f, 0});
    scaleBoneTransform("RightKnee", "RightAnkle", {0, 1.0f, 0});
    scaleBoneTransform("LeftHip", "LeftKnee", {0, 1.0f, 0});
    scaleBoneTransform("LeftKnee", "LeftAnkle", {0, 1.0f, 0});
}

void VROBodyIKController::scaleBoneTransform(std::string joint,
                                                  std::string subJoint,
                                                  VROVector3f scaleDir) {
    // Now grab the difference and determine growth ratio
    float mlShoulderLength = _mlBoneLengths[subJoint];
    float modelShoudlerLength = _modelBoneLengths[subJoint];
    float growthRatio = mlShoulderLength / modelShoudlerLength;

    // Now grow all the bones in between and modify the skinner.
    int jointIndex = _skeleton->getBone(joint)->getIndex();
    int subJointIndex = _skeleton->getBone(subJoint)->getIndex();
    _skeleton->scaleBoneTransforms(jointIndex, subJointIndex, growthRatio, scaleDir);
}

void VROBodyIKController::calculateInferredBoneSizes() {
    std::shared_ptr<VRONode> parentNode = _modelRootNode->getParentNode();
    _modelRootNode->computeTransforms(parentNode->getWorldTransform(), parentNode->getWorldRotation());

    // Calculate an inferred ml collarBone transform needed for bone length calculation
    VROMatrix4f modelNeck  = _skeleton->getCurrentBoneWorldTransform(kVROBodyBoneTags.at(VROBodyJointType::Neck));
    VROVector3f modelNeckTranslation = modelNeck.extractTranslation();
    VROMatrix4f modelCollarBone  = _skeleton->getCurrentBoneWorldTransform("mixamorig:Spine2");
    VROVector3f modelCollerBoneTranslation = modelCollarBone.extractTranslation();
    VROVector3f modelNeckToCollarBone = modelCollerBoneTranslation - modelNeckTranslation;
    VROMatrix4f mlNeckTransform = _cachedTrackedJoints[VROBodyJointType::Neck].getProjectedTransform();
    VROVector3f mlNeckTranslation = mlNeckTransform.extractTranslation();
    VROVector3f inferredMlCollarBoneTranslation = mlNeckTranslation + modelNeckToCollarBone;

    // Calculate an inferred ml hip root position needed for bone length calculation.
    VROMatrix4f modelHip = _skeleton->getCurrentBoneWorldTransform("mixamorig:Hips");
    VROVector3f modelHipTranslation = modelHip.extractTranslation();
    VROVector3f modelCollarBoneToHip = modelHipTranslation - modelCollerBoneTranslation;
    VROVector3f inferredMlRootHipTranslation = inferredMlCollarBoneTranslation + modelCollarBoneToHip;

    // Now recalibrate the proportional sizes of the remainder of the bone lengths
    // within the torso. Calculate collarbone to shoulder bone lengths.
    updateBoneLengthReference(modelCollerBoneTranslation,
                              inferredMlCollarBoneTranslation, VROBodyJointType::RightShoulder);
    updateBoneLengthReference(modelCollerBoneTranslation,
                              inferredMlCollarBoneTranslation, VROBodyJointType::LeftShoulder);

    // Calculate collarbone to neck bone lengths
    updateBoneLengthReference(modelCollerBoneTranslation,
                              inferredMlCollarBoneTranslation, VROBodyJointType::Neck);

    // Calculate collarbone to the hipRoot bone lengths
    float modelBoneDistanceHip = modelCollerBoneTranslation.distanceAccurate(modelHipTranslation);
    float mlBoneDistanceHip = inferredMlCollarBoneTranslation.distanceAccurate(inferredMlRootHipTranslation);
    _modelBoneLengths["mixamorig:Spine2"] = modelBoneDistanceHip;
    _mlBoneLengths["mixamorig:Spine2"] = mlBoneDistanceHip;

    // Calculate hipRoot bone to the right hip bone lengths.
    updateBoneLengthReference(inferredMlRootHipTranslation,
                              inferredMlRootHipTranslation, VROBodyJointType::RightHip);

    // Calculate hipRoot bone to the left hip bone lengths.
    updateBoneLengthReference(inferredMlRootHipTranslation,
                              inferredMlRootHipTranslation, VROBodyJointType::LeftHip);
}

void VROBodyIKController::updateBoneLengthReference(VROVector3f previousModelBoneTrans,
                                                         VROVector3f previousMlBoneTrans,
                                                         VROBodyJointType targetBone) {
    VROMatrix4f targetBoneModelTransform = _skeleton->getCurrentBoneWorldTransform(kVROBodyBoneTags.at(targetBone));
    VROMatrix4f targetBoneMlTransform = _cachedTrackedJoints[targetBone].getProjectedTransform();

    VROVector3f targetBoneModelTranslation = targetBoneModelTransform.extractTranslation();
    VROVector3f targetBoneMlTranslation = targetBoneMlTransform.extractTranslation();

    float modelBoneDistance = targetBoneModelTranslation.distanceAccurate(previousModelBoneTrans);
    float mlBoneDistance = targetBoneMlTranslation.distanceAccurate(previousMlBoneTrans);

    _modelBoneLengths[kVROBodyBoneTags.at(targetBone)] = modelBoneDistance;
    _mlBoneLengths[kVROBodyBoneTags.at(targetBone)] = mlBoneDistance;
}

void VROBodyIKController::calculateKnownBoneSizes(VROBodyJointType joint) {
    std::vector<VROBodyJointType> childJointsVec = kVROMLBodyTree.at(joint);
    if (childJointsVec.size() == 0) {
        return;
    }

    // Grab the current joint's position in world space.
    VROVector3f currentJointPosML
            = _cachedTrackedJoints[joint].getProjectedTransform().extractTranslation();

    // Grab the current joint's position in world space.
    std::string currentId = kVROBodyBoneTags.at(joint);
    VROVector3f currentJointPosModel
            = _skeleton->getCurrentBoneWorldTransform(currentId).extractTranslation();

    // And then calculate the bone length of each child joint.
    for (auto childJoint : childJointsVec) {
        // Calculate the ML Bone lengths
        VROVector3f childJointPosML
                = _cachedTrackedJoints[childJoint].getProjectedTransform().extractTranslation();
        float boneLengthML = currentJointPosML.distanceAccurate(childJointPosML);
        _mlBoneLengths[kVROBodyBoneTags.at(childJoint)] = boneLengthML;

        // Calculate Model Bone lengths.
        std::string childId = kVROBodyBoneTags.at(childJoint);
        VROVector3f childJointPosModel
                = _skeleton->getCurrentBoneWorldTransform(childId).extractTranslation();
        float boneLengthModel = currentJointPosModel.distanceAccurate(childJointPosModel);
        _modelBoneLengths[kVROBodyBoneTags.at(childJoint)] = boneLengthModel;

        calculateKnownBoneSizes(childJoint);
    }
}

void VROBodyIKController::onBodyPlaybackStarting(std::shared_ptr<VROBodyAnimData> animData) {
    // Matrix represented the start root world transform of the model when recording occurred.
    VROMatrix4f playbackDataStartMatrix = animData->getModelStartWorldMatrix();

    if (_rig == NULL) {
        _mlBoneLengths = animData->getBoneLengths();
        calibrateBoneProportionality();
        _rig = std::make_shared<VROIKRig>(_skeleton, _keyToEffectorMap);
    }

    setBodyTrackedState(VROBodyTrackedState::FullEffectors);
    _modelRootNode->setIKRig(_rig);
    calibrateMlToModelRootOffset();
    _playbackRootStartMatrix = _modelRootNode->getWorldTransform();

    /*
     Multiply the model world start matrix(_playbackRootStartMatrix) by the inverse of the recording
     start world matrix(the recording world's local matrix). This gives us the transform needed to
     convert world space coordinates in the recorded data to world space coordinates in the current
     model. Because our matricies are column major ordered, the inverse of the recorded world matrix is
     multiplied by the current model world matrix to give the proper result.
     */
    _playbackDataFinalTransformMatrix = _playbackRootStartMatrix * playbackDataStartMatrix.invert();
}

void VROBodyIKController::onBodyJointsPlayback(const std::map<VROBodyJointType, VROVector3f> &joints, VROBodyPlayerStatus status) {
    for (auto &latestjointPair : joints) {
        VROVector3f recordingWorldSpace = latestjointPair.second;

        // multiple recorded vector by _playbackDataFinalTransformMatrix to get coordinate into current world space.
        VROVector3f worldSpaceJoint = _playbackDataFinalTransformMatrix.multiply(recordingWorldSpace);
        VROBodyJoint bodyJoint(latestjointPair.first, 1.0f);

        std::string boneName = kVROBodyBoneTags.at(latestjointPair.first);
        _cachedModelJoints[latestjointPair.first] = worldSpaceJoint;
    }

    // Update the root motion of the rig.
    alignModelRootToMLRoot();
    // update the joints again with proper positions.

    std::map<VROBodyJointType, VROVector3f>::const_iterator cachedJoint;
    for (cachedJoint = _cachedModelJoints.begin(); cachedJoint != _cachedModelJoints.end(); cachedJoint++)
    {
        VROVector3f pos =  cachedJoint->second;
        VROBodyJointType boneMLJointType = cachedJoint->first;
        std::string boneName = kVROBodyBoneTags.at(boneMLJointType);
        _rig->setPositionForEffector(boneName, pos);
    }

    // Render debug UI
    if (_displayDebugCubes && _debugBoxEffectors.size() > 0) {
        _debugBoxRoot->setWorldTransform(_modelRootNode->getWorldPosition(), _modelRootNode->getWorldRotation());

        std::map<VROBodyJointType, VROVector3f>::const_iterator debugJoint;
        VROMatrix4f identity = VROMatrix4f::identity();
        for (debugJoint = _cachedModelJoints.begin(); debugJoint != _cachedModelJoints.end(); debugJoint++) {
            VROVector3f pos = debugJoint->second;
            VROBodyJointType boneMLJointType = debugJoint->first;
            if (isIgnoredJoint(boneMLJointType)){
                continue;
            }
            _debugBoxEffectors[boneMLJointType]->setWorldTransform(pos, identity);
        }
    }
}

void VROBodyIKController::onBodyJointsFound(const VROPoseFrame &inferredJoints) {
    if (_modelRootNode == nullptr) {
        return;
    }

    // Convert to VROBodyJoint data structure, using only first joint of each type
    std::map<VROBodyJointType, VROBodyJoint> joints;
    for (int i = 0; i < kNumBodyJoints; i++) {
        auto &kv = inferredJoints[i];
        if (!kv.empty()) {
            VROInferredBodyJoint inferred = kv[0];
            
            VROBodyJoint joint = { inferred.getType(), inferred.getConfidence() };
            joint.setScreenCoords({ inferred.getBounds().getX(), inferred.getBounds().getY(), 0 });
            joint.setSpawnTimeMs(VROTimeCurrentMillis());
            joints[(VROBodyJointType) i] = joint;
        }
    }

    // Filter new joints found given by the VROBodyTracker and update _cachedTrackedJoints
    processJoints(joints);

#if VRO_PLATFORM_IOS
    updateDebugMLViewIOS(joints);
#endif

    // Ensure we at least have the root ml joint before updating our model (neck)
    if (_currentTrackedState != NotAvailable) {
        // Only update the model if we have the required scalable joints (hips)
        if (_currentTrackedState != NoScalableJointsAvailable) {
            // Reset the model and bones back to it's initial configuration
            std::shared_ptr<VRONode> parentNode = _modelRootNode->getParentNode();
            _modelRootNode->setScale(VROVector3f(1, 1, 1));
            _modelRootNode->setRotation(VROQuaternion());
            _modelRootNode->setPosition(kInitialModelPos);
            _modelRootNode->computeTransforms(parentNode->getWorldTransform(),
                                              parentNode->getWorldRotation());

            // Dynamically scale the model to the right size.
            calibrateModelToMLTorsoScale();

            // Then determine the transform offset from an ML joint in the skeleton to the model's root.
            calibrateMlToModelRootOffset();

            // Now apply that offset and align the 3D model to the latest ML body joint positions.
            alignModelRootToMLRoot();
        }

        // Only calibrate the rig with the results if we haven't yet done so.
        if (_calibrating && _shouldCalibrateRigWithResults) {
            calibrateRigWithResults();

            // If we are calibrating without scale joints, we may not have found
            // the hips yet. Set a reasonable scale for now.
            if (_currentTrackedState == NoScalableJointsAvailable) {
                float fixedUserTorsoHeight = 0.45; // Average torso height.
                float modelToMLRatio = fixedUserTorsoHeight / _skeletonTorsoHeight * kAutomaticSizingRatio;
                _modelRootNode->setScale(VROVector3f(modelToMLRatio, modelToMLRatio, modelToMLRatio));
            }
        }
    }

    // Always notify our delegates with the latest set of joint data.
    notifyOnJointUpdateDelegates();

    // Render debug UI
    if (_displayDebugCubes && _debugBoxEffectors.size() > 0) {
        _debugBoxRoot->setWorldTransform(_modelRootNode->getWorldPosition(), _modelRootNode->getWorldRotation());

        // Render debug joint cubes.
        std::map<VROBodyJointType, VROVector3f>::const_iterator cachedJoint;
        for (auto &cachedJoint : _cachedModelJoints) {
            VROVector3f pos = cachedJoint.second;
            VROBodyJointType boneMLJointType = cachedJoint.first;
            if (isIgnoredJoint(boneMLJointType)){
                continue;
            }
            _debugBoxEffectors[boneMLJointType]->setWorldTransform(pos,  VROMatrix4f::identity());
        }
    }
}

void VROBodyIKController::notifyOnJointUpdateDelegates() {
    std::shared_ptr<VROBodyIKControllerDelegate> delegate = _delegate.lock();
    if (delegate == nullptr) {
        return;
    }

    // Construct a map containing filtered cached ML joints before dampening.
    std::map<VROBodyJointType, VROBodyIKControllerDelegate::VROJointPos> mlJointsFitlered;
    for (auto &mlJointPair : _cachedTrackedJoints) {
        VROBodyIKControllerDelegate::VROJointPos filteredJoint;
        VROBodyJoint bodyJoint = mlJointPair.second;
        filteredJoint.screenPosX = bodyJoint.getScreenCoords().x;
        filteredJoint.screenPosY = bodyJoint.getScreenCoords().y;
        filteredJoint.worldPosition = bodyJoint.getProjectedTransform().extractTranslation();
        mlJointsFitlered[mlJointPair.first] = filteredJoint;
    }

    // Construct a map containing dampened ML positional joint data.
    std::map<VROBodyJointType, VROVector3f> mlJointsDampened;
    for (auto &mlJoint : _cachedModelJoints) {
        mlJointsDampened[mlJoint.first] = mlJoint.second;
    }

    // Construct a map containing the current locations of our model joints.
    std::map<VROBodyJointType, VROMatrix4f> modelJoints;
    for (auto &jointTag : kVROBodyBoneTags) {
        VROMatrix4f worldTransform = _skeleton->getCurrentBoneWorldTransform(jointTag.second);
        modelJoints[jointTag.first] = worldTransform;
    }

    delegate->onJointUpdate(mlJointsFitlered, mlJointsDampened, modelJoints);
}

void VROBodyIKController::startRecording() {
    _isRecording = true;
    _initRecordWorldTransformOfRootNode = _modelRootNode->getWorldTransform();
#if VRO_PLATFORM_IOS
    _animDataRecorder = std::make_shared<VROBodyAnimDataRecorderiOS>();
#endif
    if (_animDataRecorder) {
        _animDataRecorder->startRecording(_initRecordWorldTransformOfRootNode, _mlBoneLengths);
    }
}

std::string VROBodyIKController::stopRecording() {
    _isRecording = false;
    _animDataRecorder->stopRecording();
    return (_animDataRecorder->toJSON());
}

void VROBodyIKController::processJoints(const std::map<VROBodyJointType, VROBodyJoint> &joints) {
    // Grab all the 2D joints of high confidence for the targets we want.
    std::map<VROBodyJointType, VROBodyJoint> latestJoints;
    for (auto &kv : joints) {
        latestJoints[kv.first] = kv.second;
    }

    // First, convert the joints into 3d space.
    projectJointsInto3DSpace(latestJoints);

    // Then, perform filtering and update our known set of cached joints.
    updateCachedJoints(latestJoints);

    // With the new found joints, update the current tracking state
    bool hasRequiredJoints = true;

    // First examine if we have the joints needed for positioning and scaling.
    if (_cachedTrackedJoints.find(VROBodyJointType::Neck) == _cachedTrackedJoints.end()) {
        hasRequiredJoints = false;
    }

    // Then, examine if we have the joints needed for scaling (right + left) or (Top) joints.
    bool hasHipJoints = true;
    if (_cachedTrackedJoints.find(VROBodyJointType::RightHip) == _cachedTrackedJoints.end() ||
        _cachedTrackedJoints.find(VROBodyJointType::LeftHip) == _cachedTrackedJoints.end())  {
        hasHipJoints = false;
    }
    if (!hasRequiredJoints) {
        setBodyTrackedState(VROBodyTrackedState::NotAvailable);
    } else if (!hasHipJoints) {
        setBodyTrackedState(VROBodyTrackedState::NoScalableJointsAvailable);
    } else if (_cachedTrackedJoints.size() == _mlJointForBoneIndex.size()) {
        setBodyTrackedState(VROBodyTrackedState::FullEffectors);
    } else if (_cachedTrackedJoints.size() >= 4) {
        setBodyTrackedState(VROBodyTrackedState::LimitedEffectors);
    }

    // Finally update _cachedModelJoints to be set on the IKRig.
    _cachedModelJoints.clear();
    
    // If we are not dampening, simply set the _cachedModelJoints and return.
    for (auto &jointPair : _cachedTrackedJoints) {
        VROVector3f pos = jointPair.second.getProjectedTransform().extractTranslation();
        _cachedModelJoints[jointPair.first] = pos;
    }
}

void VROBodyIKController::projectJointsInto3DSpace(std::map<VROBodyJointType, VROBodyJoint> &latestJoints) {
    if (latestJoints.find(kArHitTestJoint) == latestJoints.end()) {
        return;
    }

    if (kUseTorsoClusteredDepth) {
        findDampenedTorsoClusteredDepth(latestJoints);
    } else {
        // Always project the depth to a known position in space.
        VROVector3f camPos = _renderer->getCamera().getPosition();
        VROVector3f camForward = _renderer->getCamera().getForward();
        VROVector3f finalPos = camPos + (camForward * kUsePresetDepthDistanceMeter);

        _projectedPlanePosition = finalPos;
        _projectedPlaneNormal = (camPos - _projectedPlanePosition).normalize();
    }

    if (_calibrating) {
        finishCalibration(true);
    }

    // Project the 2D joints into 3D coordinates as usual.
    if (_calibrating || _hasValidProjectedPlane) {
        for (auto &joint : latestJoints) {
            VROMatrix4f hitTransform;
            float pointX = joint.second.getScreenCoords().x;
            float pointY = joint.second.getScreenCoords().y;
            bool success = performUnprojectionToPlane(pointX, pointY, hitTransform);
            if (!success) {
                joint.second.clearProjectedTransform();
            } else {
                joint.second.setProjectedTransform(hitTransform);
            }
        }
    }

    // Remove points that have failed projections from the map of latestJoints
    for (auto it = latestJoints.cbegin(), next_it = it; it != latestJoints.cend(); it = next_it) {
        ++next_it;
        if (!it->second.hasValidProjectedTransform()) {
            latestJoints.erase(it);
        }
    }
}

void VROBodyIKController::findDampenedTorsoClusteredDepth(std::map<VROBodyJointType, VROBodyJoint> &latestJoints){
    // Perform a window depth test around the body joint root to get an average Z depth.
    VROMatrix4f tempTrans = VROMatrix4f::identity();
    if (!findTorsoClusteredDepth(latestJoints, tempTrans)) {
        latestJoints.clear();
        return;
    }

    /*
     Okay consider calibration finished when... the last value set is equal to at least 75% of the previous 20 values (15).
     Also, don't set a value... if at least 3 of the last 5 don't match it.
     */
    VROVector3f camPos = _renderer->getCamera().getPosition();
    std::vector<float> distances;
    for (int i = 0; i < _candidatePlanePositions.size(); i++) {
        distances.push_back(camPos.distance(_candidatePlanePositions[i]));
    }

    float distanceToCandidate = camPos.distance(tempTrans.extractTranslation());
    bool shouldSetCurrentValue = false;
    if (_candidatePlanePositions.size() >= 5) {
        int similarCount = 0;
        for (int i = (int)_candidatePlanePositions.size() - 5; i < _candidatePlanePositions.size(); i++) {
            if (abs(distances[i] - distanceToCandidate) < .2) {
                similarCount++;
            }
        }
        if (similarCount >= 3) {
            shouldSetCurrentValue = true;
        }
    } else {
        shouldSetCurrentValue = true;
    }

    if (shouldSetCurrentValue) {
        // Update our projection plane
        _projectedPlanePosition = tempTrans.extractTranslation();
        _projectedPlaneNormal = (camPos - _projectedPlanePosition).normalize();
    }

    if (_candidatePlanePositions.size() == 10) {

        int similarCount = 0;
        for (int i = 0; i < distances.size(); i++) {
            if (abs(distances[i] - distanceToCandidate) < .2) {
                similarCount++;
            }
        }

        if (similarCount >= 7) {
            finishCalibration(true);
        }

        _candidatePlanePositions.erase(_candidatePlanePositions.begin());
    }
    _candidatePlanePositions.push_back(tempTrans.extractTranslation());
}

void VROBodyIKController::updateCachedJoints(std::map<VROBodyJointType, VROBodyJoint> &latestJoints) {
    _cachedTrackedJoints.clear();
    for (auto &latestjointPair : latestJoints) {
        _cachedTrackedJoints[latestjointPair.first] = latestjointPair.second;
    }
}

void VROBodyIKController::restoreMissingJoints(std::vector<VROBodyJoint> expiredJoints) {
    // Ignore if we are currently calibrating the rig.
    if (_calibrating) {
        return;
    }

    // We can only attempt restoration if we have the required basic joints.
    if (_currentTrackedState == VROBodyTrackedState::NotAvailable) {
        return;
    }

    // Attempt to recover missing joints by using old cached joint transforms.
    for (auto &expiredJoint : expiredJoints) {
        if (_cachedTrackedJoints.find(expiredJoint.getType()) != _cachedTrackedJoints.end()) {
            continue;
        }

        VROBodyJointType currentType = expiredJoint.getType();

        // Restore by repositioning this joint from when we last saw it relative to the root.
        VROMatrix4f cacheJointTransFromRoot = _cachedEffectorRootOffsets[currentType];
        VROMatrix4f rootTransJoint = _cachedTrackedJoints.find(kRequiredJoints[0])->second.getProjectedTransform();
        VROMatrix4f jointTrans = rootTransJoint.multiply(cacheJointTransFromRoot);
        expiredJoint.setProjectedTransform(jointTrans);
        _cachedTrackedJoints[currentType] = expiredJoint;
    }

    // With the updated transforms, cache a known set of _cachedEffectorRootOffsets
    if (_currentTrackedState != VROBodyTrackedState::NotAvailable) {
        _cachedEffectorRootOffsets.clear();
        VROMatrix4f rootJointTrans = _cachedTrackedJoints[kRequiredJoints[0]].getProjectedTransform();

        for (auto joint : _cachedTrackedJoints) {
            if (joint.first == VROBodyJointType::Neck) {
                continue;
            }
            VROMatrix4f jointTrans = joint.second.getProjectedTransform();
            VROMatrix4f rootToJointTrans = rootJointTrans.invert().multiply(jointTrans);
            _cachedEffectorRootOffsets[joint.first] = rootToJointTrans;
        }
    }
}

void VROBodyIKController::calibrateMlToModelRootOffset() {
    // Then ensure the whole model model is computed before getting world transforms.
    std::shared_ptr<VRONode> parentNode = _modelRootNode->getParentNode();
    _modelRootNode->computeTransforms(parentNode->getWorldTransform(), parentNode->getWorldRotation());

    /*
     Here, we chose an mlJoint as our "rootMotionJoint" from which to refer to when
     moving the root position of the IK Rig in world space. The idea is to calculate
     a transform - _mlRootToModelRoot - of which to apply on the rootMotionJoint as
     the user moves, to then find the new position of the 3D model's root node.

     To calculate _mlRootToModelRoot, we firstly grab the rootMotionJoint's referenced
     skeleton bone - rootMotionBone - by looking up the bone's id for the given rootMotionJoint
     from the _mlJointForBoneIndex map. We then calculate the transform offset from this
     rootMotionBone to the 3D model's root node, and save the final result to _mlRootToModelRoot.

     During tracking, this transform offset is then re-applied onto the rootMotionJoint
     to get the model's new root node position within alignModelRootToMLRoot().

     Note that below, instead of a "rootMotionJoint", we will use the position between
     the hips for calculating the transform offset.
    */
    int leftHipBoneIndex = _mlJointForBoneIndex[VROBodyJointType::LeftHip];
    int rightHipBoneIndex = _mlJointForBoneIndex[VROBodyJointType::RightHip];
    VROVector3f start = _skeleton->getCurrentBoneWorldTransform(leftHipBoneIndex).extractTranslation();
    VROVector3f end = _skeleton->getCurrentBoneWorldTransform(rightHipBoneIndex).extractTranslation();
    VROVector3f mid = (start - end).scale(0.5f) + end;

    VROMatrix4f mlRootWorldTrans;
    mlRootWorldTrans.translate(mid);
    VROMatrix4f modelRootWorldTrans; //450000
    modelRootWorldTrans.translate(_modelRootNode->getWorldTransform().extractTranslation());

    _mlRootToModelRoot = mlRootWorldTrans.invert().multiply(modelRootWorldTrans);
}

void VROBodyIKController::alignModelRootToMLRoot() {
    // Grab the world transform of what we consider to be the Body Joint's Root.
    VROVector3f bodyJointRootposition = getMLRootPosition();

    // Note: we just want the translational part of the ML's root transform as scale
    // and rotation doesn't matter at this point - they will be taken into account in the IKRig.
    VROMatrix4f bodyJointRootTransformTranslation;
    bodyJointRootTransformTranslation.translate(bodyJointRootposition);

    // Calculate the model's desired root location by multiplying the precalculated
    // _mlRootToModelRoot given the current bodyJointRootTransform.
    VROMatrix4f modelRootTransform = bodyJointRootTransformTranslation.multiply(_mlRootToModelRoot);

    // Update the model's node.
    VROVector3f pos = modelRootTransform.extractTranslation();
    VROQuaternion rot = modelRootTransform.extractRotation(modelRootTransform.extractScale());
    _modelRootNode->setWorldTransform(pos, rot, false);
}

void VROBodyIKController::calculateSkeletonTorsoDistance() {
    // Set the model in it's original scale needed for determining ratios.
    _modelRootNode->setScale(VROVector3f(1, 1, 1));
    std::shared_ptr<VRONode> parentNode = _modelRootNode->getParentNode();
    _modelRootNode->computeTransforms(parentNode->getWorldTransform(), parentNode->getWorldRotation());

    // Now calculate the ratios for automatic resizing.
    VROMatrix4f neckTrans =
            _skeleton->getCurrentBoneWorldTransform(kVROBodyBoneTags.at(VROBodyJointType::Neck));
    VROMatrix4f leftHipTrans =
            _skeleton->getCurrentBoneWorldTransform(kVROBodyBoneTags.at(VROBodyJointType::LeftHip));
    VROMatrix4f rightHipTrans =
            _skeleton->getCurrentBoneWorldTransform(kVROBodyBoneTags.at(VROBodyJointType::RightHip));

    // Now get the middle of the hip.
    VROVector3f midVecFromLeft = (rightHipTrans.extractTranslation() - leftHipTrans.extractTranslation()).scale(0.5f);
    VROVector3f midHipLoc = leftHipTrans.extractTranslation().add(midVecFromLeft);
    VROVector3f neckLoc = neckTrans.extractTranslation();
    _skeletonTorsoHeight = midHipLoc.distanceAccurate(neckLoc);
}

void VROBodyIKController::calibrateModelToMLTorsoScale() {
    // We'll need the current dimensions of the model for resizing calculations.
    if (!kAutomaticResizing || _skeleton == nullptr) {
        return;
    }

    // Calculate the neckToMLRoot distance
    VROVector3f neckPos = _cachedModelJoints[VROBodyJointType::Neck];
    VROVector3f midHipLoc = getMLRootPosition();
    _userTorsoHeight = midHipLoc.distanceAccurate(neckPos);

    // Calculate the different distances, grab the ratio.
    float modelToMLRatio = _userTorsoHeight / _skeletonTorsoHeight * kAutomaticSizingRatio;

    // Apply that ratio to the scale of the model.
    _modelRootNode->setScale(VROVector3f(modelToMLRatio, modelToMLRatio, modelToMLRatio));
}

VROVector3f VROBodyIKController::getMLRootPosition() {
    if (_currentTrackedState == VROBodyTrackedState::NotAvailable) {
        pwarn("Unable to determine ML Root position without proper body tracking data.");
        return VROVector3f();
    }

    VROVector3f start = _cachedModelJoints[VROBodyJointType::LeftHip];
    VROVector3f end = _cachedModelJoints[VROBodyJointType::RightHip];
    return (start - end).scale(0.5f) + end;
}

void VROBodyIKController::updateModel() {

    if (_rig == nullptr || _currentTrackedState == VROBodyTrackedState::NotAvailable) {
        return;
    }

    // Update the root motion of the rig.
    alignModelRootToMLRoot();

    if (_isRecording && _animDataRecorder) {
        _animDataRecorder->beginRecordedRow();
    }

    // Now update all known rig joints.
    std::map<VROBodyJointType, VROVector3f>::const_iterator cachedJoint;
    for (auto &cachedJoint : _cachedModelJoints) {
        VROVector3f pos = cachedJoint.second;
        VROBodyJointType boneMLJointType = cachedJoint.first;
        if (isIgnoredJoint(boneMLJointType)) {
            continue;
        }

        std::string boneName = kVROBodyBoneTags.at(boneMLJointType);
        _rig->setPositionForEffector(boneName, pos);

        if (_isRecording && _animDataRecorder) {
            _animDataRecorder->addJointToRow(boneName, pos);
        }
    }

    if (_isRecording && _animDataRecorder) {
        _animDataRecorder->endRecordedRow();
    }
}

bool VROBodyIKController::findTorsoClusteredDepth(std::map<VROBodyJointType, VROBodyJoint> &latestJoints, VROMatrix4f &matOut) {
    std::shared_ptr<VROARSession> arSession;
    
#if VRO_PLATFORM_IOS
    arSession = [_view getARSession];
#endif
    if (!arSession) {
        return false;
    }
    
    std::unique_ptr<VROARFrame> &lastFrame = arSession->getLastFrame();
    std::vector<VROVector4f> pointCloudPoints = lastFrame->getPointCloud()->getPoints();
    
    
    // get the "box" around the user's torso based on a diagonal pair of hip & shoulders
    float maxX;
    float minX;
    float maxY;
    float minY;
    
    auto rightShoulderIt = latestJoints.find(VROBodyJointType::RightShoulder);
    auto leftShoulderIt = latestJoints.find(VROBodyJointType::LeftShoulder);
    auto rightHipIt = latestJoints.find(VROBodyJointType::RightHip);
    auto leftHipIt = latestJoints.find(VROBodyJointType::LeftHip);

    if (rightShoulderIt != latestJoints.end() && leftHipIt != latestJoints.end()) {
        maxX = leftHipIt->second.getScreenCoords().x;
        minX = rightShoulderIt->second.getScreenCoords().x;
        maxY = leftHipIt->second.getScreenCoords().y;
        minY = rightShoulderIt->second.getScreenCoords().y;
    } else if (leftShoulderIt != latestJoints.end() && rightHipIt != latestJoints.end()) {
        maxX = leftShoulderIt->second.getScreenCoords().x;
        minX = rightHipIt->second.getScreenCoords().x;
        maxY = rightHipIt->second.getScreenCoords().y;
        minY = leftShoulderIt->second.getScreenCoords().y;
        
    } else {
        return false;
    }

    float torsoWidth = maxX - minX;
    float torsoHeight = maxY - minY;
    
    std::vector<VROVector3f> hitTestResults;
    
    for (int i = 0; i < 10; i++) {
        VROMatrix4f estimate;
        if (performDepthTest(minX + (torsoWidth * ((float) rand()) / RAND_MAX), minY + (torsoHeight * ((float) rand()) / RAND_MAX), estimate)) {
            hitTestResults.push_back(estimate.extractTranslation());
        }
    }
    
    if (hitTestResults.size() <= 5) {
        return false;
    }
    
    matOut.translate(findClusterInPoints(hitTestResults));
    return true;
}

VROVector3f VROBodyIKController::findClusterInPoints(std::vector<VROVector3f> points) {
    /*
     The algorithm we use here isn't difficult... we just grab the median value and ignore
     all points that are more than .3 meters from it. This is because we're getting values like:
     
     2.2342, 2.452, 2.223, 2.3334, 12.2343, 2.341, 5.2342, etc.
     
     So by sorting all the values and grabbing the median value, we "throw" away all the artifacts at the edges
     */
    VROVector3f cameraPos = _renderer->getCamera().getPosition();

    std::vector<float> distances;
    for (int i = 0; i < points.size(); i++) {
        distances.push_back(cameraPos.distance(points[i]));
    }
    
    std::sort(distances.begin(), distances.end());
    float midpoint = distances[distances.size() / 2];
    
    VROVector3f totalPosition = {0,0,0};
    for (int i = 0; i < distances.size();) {
        if (abs(midpoint - distances[i]) > .3) {
            distances.erase(distances.begin() + i);
            points.erase(points.begin() + i);
        } else {
            totalPosition += points[i];
            i++;
        }
    }
    
    totalPosition /= distances.size();
    return totalPosition;
}

bool VROBodyIKController::performWindowDepthTest(float x, float y, VROMatrix4f &matOut) {
    float d = kARHitTestWindowKernelPixel;
    std::vector<VROVector3f> trials;
    trials.push_back(VROVector3f(x,     y, 0));
    trials.push_back(VROVector3f(x + d, y - d, 0));
    trials.push_back(VROVector3f(x - d, y - d, 0));
    trials.push_back(VROVector3f(x + d, y + d, 0));
    trials.push_back(VROVector3f(x - d, y + d, 0));

    VROVector3f total;
    float count = 0;
    for (auto t : trials) {
        VROMatrix4f estimate;
        if (performDepthTest(t.x, t.y, estimate)) {
            count = count + 1;
            total = total.add(estimate.extractTranslation());
        }
    }

    if (count == 0) {
        return false; // Hit test has failed.
    }

    total = total / count;
    matOut.toIdentity();
    matOut.translate(total);
    return true;
}

bool VROBodyIKController::performDepthTest(float x, float y, VROMatrix4f &matOut) {
    // Else use the usual ARHitTest to determine depth.
    float _currentProjection = 0.0037;
    int viewportWidth =_renderer->getCamera().getViewport().getWidth();
    int viewportHeight = _renderer->getCamera().getViewport().getHeight();
    std::vector<std::shared_ptr<VROARHitTestResult>> results;
#if VRO_PLATFORM_IOS
    VROVector3f transformed = { x * viewportWidth, y * viewportHeight, _currentProjection };
    results = [_view performARHitTestWithPoint:transformed.x y:transformed.y];
#endif

    std::shared_ptr<VROARHitTestResult> finalResult = nullptr;
    for (auto result: results) {
        if (finalResult == nullptr) {
            finalResult = result;
            continue;
        }

        // only consider feature points and choose the closest one.
        if (result->getType() == VROARHitTestResultType::FeaturePoint && result->getDistance() < finalResult->getDistance()) {
            finalResult = result;
        }
    }

    if (finalResult == nullptr) {
        return false;
    } else {
        VROVector3f pos = finalResult->getWorldTransform().extractTranslation();
        VROMatrix4f out = VROMatrix4f::identity();
        out.translate(pos);
        matOut = out;
        return true;
    }
}


bool VROBodyIKController::performUnprojectionToPlane(float x, float y, VROMatrix4f &matOut) {
    const VROCamera &camera = _renderer->getCamera();
    int viewport[4] = {0, 0, camera.getViewport().getWidth(), camera.getViewport().getHeight()};
    VROMatrix4f mvp = camera.getProjection().multiply(camera.getLookAtMatrix());
    x = viewport[2] * x;
    y = viewport[3] * y;

    // Compute the camera ray by unprojecting the point at the near clipping plane
    // and the far clipping plane.
    VROVector3f ncpScreen(x, y, 0.0);
    VROVector3f ncpWorld;
    if (!VROProjector::unproject(ncpScreen, mvp.getArray(), viewport, &ncpWorld)) {
        return false;
    }

    VROVector3f fcpScreen(x, y, 1.0);
    VROVector3f fcpWorld;
    if (!VROProjector::unproject(fcpScreen, mvp.getArray(), viewport, &fcpWorld)) {
        return false;
    }
    VROVector3f ray = fcpWorld.subtract(ncpWorld).normalize();

    // Find the intersection between the plane and the controller forward
    VROVector3f intersectionPoint;
    bool success = ray.rayIntersectPlane(_projectedPlanePosition, _projectedPlaneNormal, ncpWorld, &intersectionPoint);
    matOut.toIdentity();
    matOut.translate(intersectionPoint);
    return success;
}

void VROBodyIKController::setBodyTrackedState(VROBodyTrackedState state) {
    if (_currentTrackedState == state) {
        return;
    }

    _currentTrackedState = state;
    std::shared_ptr<VROBodyIKControllerDelegate> delegate = _delegate.lock();
    if (delegate != nullptr) {
        delegate->onBodyTrackStateUpdate(_currentTrackedState);
    }
}

void VROBodyIKController::setDelegate(std::shared_ptr<VROBodyIKControllerDelegate> delegate) {
    _delegate = delegate;
}

std::shared_ptr<VRONode> VROBodyIKController::createDebugBoxUI(bool isAffector, std::string tag) {
    // Create our debug box node
    float dimen = 0.005;
    if (isAffector) {
        dimen =  0.005;
    }
    std::shared_ptr<VROBox> box = VROBox::createBox(dimen, dimen, dimen);
    std::shared_ptr<VROMaterial> mat = std::make_shared<VROMaterial>();
    mat->setCullMode(VROCullMode::None);
    mat->setReadsFromDepthBuffer(false);
    mat->setWritesToDepthBuffer(false);
    if (isAffector) {
        mat->getDiffuse().setColor(VROVector4f(0.0, 1.0, 0, 1.0));
    } else {
        mat->getDiffuse().setColor(VROVector4f(1.0, 0, 0, 1.0));
    }

    std::vector<std::shared_ptr<VROMaterial>> mats;
    mats.push_back(mat);
    box->setMaterials(mats);

    std::shared_ptr<VRONode> debugNode = std::make_shared<VRONode>();
    debugNode->setGeometry(box);
    debugNode->setRenderingOrder(10);
    debugNode->setTag(tag);
    debugNode->setIgnoreEventHandling(true);

    return debugNode;
}

#if VRO_PLATFORM_IOS
void VROBodyIKController::enableDebugMLViewIOS() {
    if (_view == NULL) {
        pwarn("View is not yet setup properly, unable to enableDebugMLView!");
        return;
    }
    
    int endJointCount = static_cast<int>(VROBodyJointType::Pelvis);
    for (int i = static_cast<int>(VROBodyJointType::Top); i <= endJointCount; i++) {
        _bodyViews[i] = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 4, 4)];
        _bodyViews[i].backgroundColor = colors[i];
        _bodyViews[i].clipsToBounds = NO;

        _labelViews[i] = [[UILabel alloc] initWithFrame:CGRectMake(7, -3, 300, 8)];
        _labelViews[i].text = [NSString stringWithUTF8String:pointLabels[i].c_str()];
        _labelViews[i].textColor = colors[i];
        _labelViews[i].font = [UIFont preferredFontForTextStyle:UIFontTextStyleCaption2];
        [_labelViews[i] setFont:[UIFont systemFontOfSize:9]];
        [_bodyViews[i] addSubview:_labelViews[i]];
        [_view addSubview:_bodyViews[i]];
    }
}

void VROBodyIKController::updateDebugMLViewIOS(const std::map<VROBodyJointType, VROBodyJoint> &joints) {
    float minAlpha = 0.4;
    float maxAlpha = 1.0;
    float maxConfidence = 0.6;
    float minConfidence = 0.1;
    int viewWidth  = _view.frame.size.width;
    int viewHight = _view.frame.size.height;

    int endJointCount = static_cast<int>(VROBodyJointType::LeftAnkle);
    for (int i = static_cast<int>(VROBodyJointType::Top); i <= endJointCount; i++) {
        std::string labelTag = pointLabels[i] + " [N/A]";
        _labelViews[i].text =  [NSString stringWithUTF8String:labelTag.c_str()];
    }

    for (auto &kv : joints) {
        if ((int) kv.first > endJointCount) {
            continue;
        }
        
        VROVector3f point = kv.second.getScreenCoords();
        VROVector3f transformed = { point.x * viewWidth, point.y * viewHight, 0 };
        // Only update the text for points that match our level of confidence.
        // Note that low confidence points are still rendered to ensure validity.
        //if (kv.second.getConfidence() > _mlJointConfidenceThresholds[kv.first]) {
            std::string labelTag = pointLabels[(int)kv.first] + " -> " + kv.second.getProjectedTransform().extractTranslation().toString();
            _labelViews[(int)kv.first].text = [NSString stringWithUTF8String:labelTag.c_str()];
        //}
        
        _bodyViews[(int) kv.first].center = CGPointMake(transformed.x, transformed.y);
        _bodyViews[(int) kv.first].alpha = VROMathInterpolate(kv.second.getConfidence(), minConfidence, maxConfidence, minAlpha, maxAlpha);
    }
}
#endif

void VROBodyIKController::setDisplayDebugCubes(bool visible) {
    _displayDebugCubes = visible;
}

bool VROBodyIKController::getDisplayDebugCubes() {
    return _displayDebugCubes;
}

void VROBodyIKController::setStalenessThresholdForJoint(VROBodyJointType type, float timeoutMs) {
    _mlJointTimeoutMap[type] = timeoutMs;
}

float VROBodyIKController::getStalenessThresholdForJoint(VROBodyJointType type) {
    return _mlJointTimeoutMap[type];
}

bool VROBodyIKController::isIgnoredJoint(VROBodyJointType joint) {
    for (auto ignoredJoint : kIgnoredJoints) {
        if (joint == ignoredJoint) {
            return true;
        }
    }
    return false;
}
