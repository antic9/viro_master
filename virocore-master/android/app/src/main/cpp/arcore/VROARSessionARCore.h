//
//  VROARSessionARCore.h
//  ViroKit
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

#ifndef VROARSessionARCore_h
#define VROARSessionARCore_h

#include "VROARSession.h"
#include "VROARFrameARCore.h"
#include "VROARAnchorARCore.h"
#include "VROViewport.h"
#include "VROOpenGL.h"
#include "ARCore_API.h"
#include <map>
#include <vector>
#include <VROCameraTexture.h>
#include <VROARPlaneAnchor.h>
#include <VROARImageAnchor.h>
#include <VROARImageDatabase.h>

enum class VROARDisplayRotation {
    R0,
    R90,
    R180,
    R270
};

class VRODriverOpenGL;
class VROCloudAnchorProviderARCore;

class VROARSessionARCore : public VROARSession,
                           public std::enable_shared_from_this<VROARSessionARCore> {
public:
    
    VROARSessionARCore(std::shared_ptr<VRODriverOpenGL> driver);
    virtual ~VROARSessionARCore();

    void setTrackingType(VROTrackingType type) {}
    void run();
    void pause();
    bool isReady() const;
    void resetSession(bool resetTracking, bool removeAnchors);

    /*
     Configure this ARCore session with the given modes. Returns true if supported.
     */
    bool configure(arcore::LightingMode lightingMode, arcore::PlaneFindingMode planeFindingMode,
                   arcore::UpdateMode updateMode, arcore::CloudAnchorMode cloudAnchorMode);
    
    void setScene(std::shared_ptr<VROScene> scene);
    void setDelegate(std::shared_ptr<VROARSessionDelegate> delegate);
    bool setAnchorDetection(std::set<VROAnchorDetection> types);
    void setCloudAnchorProvider(VROCloudAnchorProvider provider);
    void setAutofocus(bool enabled);
    bool isCameraAutoFocusEnabled();

    /*
     This function loads the ARCore AugmentedImageDatabase from the given database by overwriting
     the existing one and then adding any individually added targets (from addARImageTarget).
     */
    void loadARImageDatabase(std::shared_ptr<VROARImageDatabase> arImageDatabase);
    void unloadARImageDatabase();
    void addARImageTarget(std::shared_ptr<VROARImageTarget> target);
    void removeARImageTarget(std::shared_ptr<VROARImageTarget> target);

    void addARObjectTarget(std::shared_ptr<VROARObjectTarget> target) {
        // no-op
    }

    void removeARObjectTarget(std::shared_ptr<VROARObjectTarget> target) {
        // no-op
    }


    void addAnchor(std::shared_ptr<VROARAnchor> anchor);
    void removeAnchor(std::shared_ptr<VROARAnchor> anchor);
    void updateAnchor(std::shared_ptr<VROARAnchor> anchor);
    void hostCloudAnchor(std::shared_ptr<VROARAnchor> anchor,
                         std::function<void(std::shared_ptr<VROARAnchor>)> onSuccess,
                         std::function<void(std::string error)> onFailure);
    void resolveCloudAnchor(std::string cloudAnchorId,
                            std::function<void(std::shared_ptr<VROARAnchor> anchor)> onSuccess,
                            std::function<void(std::string error)> onFailure);

    std::unique_ptr<VROARFrame> &updateFrame();
    std::unique_ptr<VROARFrame> &getLastFrame();
    std::shared_ptr<VROTexture> getCameraBackgroundTexture();
    
    void setViewport(VROViewport viewport);
    void setOrientation(VROCameraOrientation orientation);
    void setWorldOrigin(VROMatrix4f relativeTransform);

    void setNumberOfTrackedImages(int numImages) {
        // no-op (only iOS 12+)
    }


    void setVideoQuality(VROVideoQuality quality) {
        // no-op on Android
    }

    void setVisionModel(std::shared_ptr<VROVisionModel> visionModel) {
        // no-op on Android
    }

#pragma mark - [Internal] Configuration

    /*
     Invoked when ARCore is installed on the device: sets the ARCore session implementation.
     This object will own the session.
     */
    void setARCoreSession(arcore::Session *session, std::shared_ptr<VROFrameSynchronizer> synchronizer);

    arcore::Session *getSessionInternal() {
        return _session;
    }

#pragma mark - [Internal] Camera Background

    GLuint getCameraTextureId() const;

    /*
     Initialize the camera background texture and install it on the ARCore session.
     */
    void initCameraTexture(std::shared_ptr<VRODriverOpenGL> driver);

    void setDisplayGeometry(VROARDisplayRotation rotation, int width, int height);
    VROARDisplayRotation getDisplayRotation() const {  return _displayRotation; }
    int getWidth() const { return _width; }
    int getHeight() const { return _height; }

    /*
     Retrieve the shared rotated camera image data array. The data must be of the
     given size in bytes.
     */
    uint8_t *getRotatedCameraImageData(int size);

#pragma mark - [Internal] Anchors

    std::shared_ptr<VROARAnchor> getAnchorWithId(std::string anchorId);
    std::shared_ptr<VROARAnchor> getAnchorForNative(arcore::Anchor *anchor);
    std::string getKeyForTrackable(arcore::Trackable *trackable);
    std::shared_ptr<VROARAnchorARCore> getAnchorForTrackable(arcore::Trackable *trackable);

private:

    /*
     The ARCore session.
     */
    arcore::Session *_session;

    /*
     Per frame handling.
     */
    std::shared_ptr<VROFrameSynchronizer> _synchronizer;

    /*
     Reusable ARCore frame object.
     */
    arcore::Frame *_frame;
    int _frameCount;

    /*
     The last computed ARFrame.
     */
    std::unique_ptr<VROARFrame> _currentFrame;
    
    /*
     The current viewport and camera orientation.
     */
    VROViewport _viewport;
    VROCameraOrientation _orientation;

#pragma mark - [Private] Configuration

    arcore::LightingMode _lightingMode;
    arcore::PlaneFindingMode _planeFindingMode;
    arcore::UpdateMode _updateMode;
    arcore::CloudAnchorMode _cloudAnchorMode;
    arcore::FocusMode _focusMode;

    bool updateARCoreConfig();

#pragma mark - [Private] ARCore Image Tracking

    arcore::AugmentedImageDatabase *_currentARCoreImageDatabase;
    std::vector<std::shared_ptr<VROARImageTarget>> _imageTargets;

    /*
     This is a helper function that synchronously adds the target to the database. This function
     should not be called on the rendering thread (as per ARCore guidance).
     */
    void addTargetToDatabase(std::shared_ptr<VROARImageTarget> target,
                             arcore::AugmentedImageDatabase *database);

    /*
     This function rotates the given grayscaleImage so that the image is "Up" based on the given
     orientation. This function sets the given pointers to their new values (keep in mind that
     the caller should free the rotated grayscaleImage when they're done with it).
     */
    void rotateImageForOrientation(uint8_t **grayscaleImage, int *width, int *height, size_t *stride,
                                   VROImageOrientation orientation);

#pragma mark - [Private] Anchor Processing

    /*
     Vector of all anchors that have been added to this session.
     */
    std::vector<std::shared_ptr<VROARAnchorARCore>> _anchors;

    /*
     Map of ARCore anchors ("native" anchors) to their Viro representation.
     Required so we can update VROARAnchors when their ARCore counterparts are
     updated.
     */
    std::map<std::string, std::shared_ptr<VROARAnchorARCore>> _nativeAnchorMap;

    /*
     Hosts and resolves cloud anchors.
     */
    std::shared_ptr<VROCloudAnchorProviderARCore> _cloudAnchorProvider;

    /*
     Per-frame anchor and trackable update handling.
     */
    void processUpdatedAnchors(VROARFrameARCore *frame);

    /*
     These methods sync Viro anchors (representing ARCore *trackables*) with their corresponding
     ARCore objects. The ARCore objects contain all the updated information.
     */
    void syncPlaneWithARCore(std::shared_ptr<VROARPlaneAnchor> plane, arcore::Plane *planeAR);
    void syncImageAnchorWithARCore(std::shared_ptr<VROARImageAnchor> imageAnchor,
                                   arcore::AugmentedImage *imageAR);

#pragma mark - [Private] Camera Background

    /*
     Background to be assigned to the VROScene.
     */
    std::shared_ptr<VROTexture> _background;

    /*
     The GL_TEXTURE_EXTERNAL_OES texture used for the camera background.
     */
    GLuint _cameraTextureId;

    /*
     The display rotation used by ARCore.
     */
    VROARDisplayRotation _displayRotation;

    /*
     The width and height used by the viewport (and corresponding camera texture).
     */
    int _width;
    int _height;

    /*
     Stores the RGBA8 rotated camera image data, each frame. This is kept here instead of in
     VROARCameraARCore so that it can be re-used each frame. VROARCameraARCore never exposes this
     to external clients, it only exposes the 'cropped' image data, which matches what's visible
     by the AR viewport.
     */
    int _rotatedImageDataLength;
    uint8_t *_rotatedImageData;

};

#endif /* VROARSessionARCore_h */
