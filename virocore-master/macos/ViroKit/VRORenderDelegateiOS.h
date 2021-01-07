//
//  VRORenderDelegateiOS.h
//  ViroRenderer
//
//  Created by Raj Advani on 11/6/16.
//  Copyright © 2016 Viro Media. All rights reserved.
//

#ifndef VRORenderDelegateiOS_h
#define VRORenderDelegateiOS_h

#import "VRORenderDelegateInternal.h"
#import "VRORenderDelegate.h"

class VRORenderDelegateiOS : public VRORenderDelegateInternal {
    
public:

    VRORenderDelegateiOS(id<VRORenderDelegate> delegate) :
        _delegate(delegate) {}
    virtual ~VRORenderDelegateiOS() {}
    
    void setupRendererWithDriver(std::shared_ptr<VRODriver> driver) {
        [_delegate setupRendererWithDriver:driver];
    }
    
    void userDidRequestExitVR() {
        [_delegate userDidRequestExitVR];
    }

private:
    
    __weak id<VRORenderDelegate> _delegate;
    
};

#endif /* VRORenderDelegateiOS_h */
