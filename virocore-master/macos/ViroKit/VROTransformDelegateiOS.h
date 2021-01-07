//
//  VROTransformDelegateiOS
//  ViroRenderer
//
//  Copyright © 2017 Viro Media. All rights eserved.
//

#ifndef VROTransformDelegateiOS_h
#define VROTransformDelegateiOS_h

#include <memory>
#include <vector>

@protocol VROTransformDelegateProtocol<NSObject>
@required
- (void)onPositionUpdate:(VROVector3f)position;
@end

/*
 Notifies the bridge regarding transformation udpates of this delegate's attached control
 */
class VROTransformDelegateiOS: public VROTransformDelegate {
public:
    VROTransformDelegateiOS(id<VROTransformDelegateProtocol> delegate, double distanceFilter): VROTransformDelegate(distanceFilter) {
        _delegate = delegate;
    }
    
    virtual ~VROTransformDelegateiOS() {}
    void onPositionUpdate(VROVector3f position) {
        [_delegate onPositionUpdate:position];
    }
    
private:
    __weak id<VROTransformDelegateProtocol> _delegate;
    
};

#endif
