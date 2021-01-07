//
//  VROFrameSynchronizerInternal.h
//  ViroRenderer
//
//  Created by Raj Advani on 7/22/16.
//  Copyright © 2016 Viro Media. All rights reserved.
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

#ifndef VROFrameSynchronizerInternal_h
#define VROFrameSynchronizerInternal_h

#include "VROFrameSynchronizer.h"
#include <vector>

class VRORenderContext;

class VROFrameSynchronizerInternal : public VROFrameSynchronizer {
    
public:
    
    virtual ~VROFrameSynchronizerInternal() {}
    
    void addFrameListener(std::shared_ptr<VROFrameListener> listener);
    void removeFrameListener(std::shared_ptr<VROFrameListener> listener);
    
    void notifyFrameStart(const VRORenderContext &context);
    void notifyFrameEnd(const VRORenderContext &context);
    
private:
    
    /*
     Listeners that receive an update each frame.
     */
    std::vector<std::weak_ptr<VROFrameListener>> _frameListeners;
    
};

#endif /* VROFrameSynchronizerInternal_h */
