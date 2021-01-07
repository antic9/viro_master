//
//  VROBodyMesheriOS.hpp
//  ViroKit
//
//  Created by Raj Advani on 5/22/19.
//  Copyright © 2019 Viro Media. All rights reserved.
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

#ifndef VROBodyMesheriOS_hpp
#define VROBodyMesheriOS_hpp

#include <memory>
#include "VROVisionEngine.h"
#include "VROBodyMesher.h"
#include "VROCameraTexture.h"
#import <Foundation/Foundation.h>
#include "VROMatrix4f.h"
#include "cnpy.h"

class VRODriver;
class VROPoseFilter;
class VROGeometry;
class VROGeometryElement;
class VROGeometrySource;
class VROOneEuroFilterF;

class API_AVAILABLE(ios(11.0)) VROBodyMesheriOS : public VROBodyMesher, public VROVisionEngineDelegate {
public:
    
    VROBodyMesheriOS();
    virtual ~VROBodyMesheriOS();
    
    bool initBodyTracking(VROCameraPosition position, std::shared_ptr<VRODriver> driver);
    void startBodyTracking();
    void stopBodyTracking();
    void update(const VROARFrame *frame);
    
    std::shared_ptr<VROGeometry> getBodyMesh() {
        return _bodyMesh;
    }
    
    /*
     Process the output of the VROVisionEngine. This will convert the raw output from CoreML into the
     body joints, pass the joints through a filter and finally invoke the delegate on the rendering
     thread.
     
     Invoked on the visionQueue.
     */
    std::vector<std::pair<VROVector3f, float>> processVisionOutput(VNCoreMLFeatureValueObservation *result, VROCameraPosition cameraPosition,
                                                                   VROMatrix4f visionToImageSpace, VROMatrix4f imageToViewportSpace);
    
    /*
     Sets the window period at which we sample points for dampening. If period == 0,
     no dampening will be applied.
     */
    void setDampeningPeriodMs(double period);
    double getDampeningPeriodMs() const;
    
    /*
     Get the dynamic crop box used for the last render.
     */
    CGRect getDynamicCropBox() const {
        return _visionEngine->getDynamicCropBox();
    }
    
    /*
     Get the filter being used to smooth the output joint data.
     */
    std::shared_ptr<VROPoseFilter> getPoseFilter() {
        return _poseFilter;
    }
    
private:
    
    /*
     Handles processing of AR frames through CoreML and back out into this object
     for handling.
     */
    std::shared_ptr<VROVisionEngine> _visionEngine;
    
    /*
     True when tracking is running; e.g. images are being fed into CoreML.
     */
    bool _isTracking;
    
    /*
     Filter used on pose data before sending to the delegate.
     */
    std::shared_ptr<VROPoseFilter> _poseFilter;
    
    /*
     Dampening window milliseconds. If period is set to 0, no dampening will be applied.
     */
    double _dampeningPeriodMs;
    
    /*
     The body mesh constructed by this controller.
     */
    std::shared_ptr<VROGeometry> _bodyMesh;
    
    /*
     UV map data for resampling.
     */
    cnpy::NpyArray _uvTexcoords;
    cnpy::NpyArray _uvMask;
    cnpy::NpyArray _uvVtoVt;
    cnpy::NpyArray _uvFaceToV;
    
    cnpy::NpyArray _testUv;
    
    /*
     Derive the vertex points from the output of the CoreML model. The given transforms go from
     vision space [0, 1] to image space [0, 1], to normalized viewport space [0, 1].
     */
    std::vector<float> deriveVertices(MLMultiArray *uvmap, VROCameraPosition cameraPosition,
                                      VROMatrix4f visionToImageSpace, VROMatrix4f imageToViewportSpace,
                                      std::pair<VROVector3f, float> *outImageSpaceJoints);
    
    /*
     Generate geometry element (faces array) for the human body mesh. This never needs to
     be updated.
     */
    std::shared_ptr<VROGeometryElement> buildMeshFaces();
    
    /*
     Converts the output from the given CoreML MLMultiArray into a full body mesh in screen
     coordinates.
     */
    std::shared_ptr<VROGeometrySource> buildMeshVertices(std::vector<float> &vertices);
    
    /*
     Get a sampling kernel that, if added to a texture coordinate, represents the box of
     samples we should take around that coordinate, in order of preference.
     */
    std::vector<std::vector<int>> getSamplingKernel(int distance);
    
    /*
     Load Numpy arrays included in the bundle.
     */
    static cnpy::NpyArray loadNumpyArray(NSString *prefix, NSString *array);
    static cnpy::NpyArray loadNumpyArray(NSString *name);
    
};

#endif
