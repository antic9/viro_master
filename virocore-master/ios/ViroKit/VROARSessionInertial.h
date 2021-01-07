//
//  VROARSessionInertial.h
//  ViroKit
//
//  Created by Raj Advani on 6/6/17.
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

#ifndef VROARSessionInertial_h
#define VROARSessionInertial_h

#include "VROARSession.h"
#include "VROViewport.h"

class VRODriver;
class VROARCameraInertial;

class VROARSessionInertial : public VROARSession, public std::enable_shared_from_this<VROARSessionInertial> {
public:
    
    VROARSessionInertial(VROTrackingType trackingType, std::shared_ptr<VRODriver> driver);
    virtual ~VROARSessionInertial();
    
    void setTrackingType(VROTrackingType trackingType) {}

    void run();
    void pause();
    bool isReady() const;
    void resetSession(bool resetTracking, bool removeAnchors);
    
    bool setAnchorDetection(std::set<VROAnchorDetection> types);
    void setCloudAnchorProvider(VROCloudAnchorProvider provider);
    void addARImageTarget(std::shared_ptr<VROARImageTarget> target);
    void removeARImageTarget(std::shared_ptr<VROARImageTarget> target);
    
    void addARObjectTarget(std::shared_ptr<VROARObjectTarget> target) {
        // unsupported
    }
    void removeARObjectTarget(std::shared_ptr<VROARObjectTarget> target) {
        // unsupported
    }
    
    
    void loadARImageDatabase(std::shared_ptr<VROARImageDatabase> arImageDatabase) {
        // unsupported
    }
    
    void unloadARImageDatabase() {
        // unsupported
    }
    
    void addAnchor(std::shared_ptr<VROARAnchor> anchor);
    void removeAnchor(std::shared_ptr<VROARAnchor> anchor);
    void updateAnchor(std::shared_ptr<VROARAnchor> anchor);
    void hostCloudAnchor(std::shared_ptr<VROARAnchor> anchor,
                         std::function<void(std::shared_ptr<VROARAnchor>)> onSuccess,
                         std::function<void(std::string error)> onFailure);
    void resolveCloudAnchor(std::string anchorId,
                            std::function<void(std::shared_ptr<VROARAnchor> anchor)> onSuccess,
                            std::function<void(std::string error)> onFailure);
    
    std::unique_ptr<VROARFrame> &updateFrame();
    std::unique_ptr<VROARFrame> &getLastFrame();
    std::shared_ptr<VROTexture> getCameraBackgroundTexture();
    
    void setViewport(VROViewport viewport);
    void setOrientation(VROCameraOrientation orientation);

    void addAnchorNode(std::shared_ptr<VRONode> node);

    void setNumberOfTrackedImages(int numImages) {
        // no-op
    }
    
    void setWorldOrigin(VROMatrix4f relativeTransform) {
        // no-op
    };
    
    void setAutofocus(bool enabled) {
        // no-op
    };
  
    bool isCameraAutoFocusEnabled() {
        // no-op
      return false;
    };

    void setVideoQuality(VROVideoQuality quality) {
        // no-op
    };
    
    void setVisionModel(std::shared_ptr<VROVisionModel> visionModel);
  
private:
    
    std::unique_ptr<VROARFrame> _currentFrame;
    std::shared_ptr<VROARCameraInertial> _camera;
    std::shared_ptr<VROVisionModel> _visionModel;
    VROViewport _viewport;
    
};

#endif /* VROARSessionInertial_h */
