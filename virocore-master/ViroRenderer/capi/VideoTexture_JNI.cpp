//
//  VideoTexture_JNI.cpp
//  ViroRenderer
//
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

#include <memory>
#include <VROVideoSurface.h>
#include <VROTextureUtil.h>
#include "VRONode.h"
#include "VROMaterial.h"
#include "VROFrameSynchronizer.h"
#include "VROPlatformUtil.h"
#include "ViroContext_JNI.h"
#include "VideoTexture_JNI.h"
#include "VideoDelegate_JNI.h"
#include "VRODriverOpenGL.h"

#if VRO_PLATFORM_ANDROID
#include "VROVideoTextureAVP.h"

#define VRO_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_com_viro_core_VideoTexture_##method_name
#else
#define VRO_METHOD(return_type, method_name) \
    return_type VideoTexture_##method_name
#endif

extern "C" {

VRO_METHOD(VRO_REF(VROVideoTexture), nativeCreateVideoTexture)(VRO_ARGS
                                                               VRO_REF(ViroContext) context_j,
                                                               VRO_STRING stereoMode) {
    VRO_METHOD_PREAMBLE;
    VROStereoMode mode = VROTextureUtil::getStereoModeForString(VRO_STRING_STL(stereoMode));
    std::weak_ptr<ViroContext> context_w = VRO_REF_GET(ViroContext, context_j);

    std::shared_ptr<VROVideoTexture> videoTexture;
#if VRO_PLATFORM_ANDROID
    std::shared_ptr<VROVideoTextureAVP> videoAVP = std::make_shared<VROVideoTextureAVP>(mode);

    VROPlatformDispatchAsyncRenderer([videoAVP, context_w] {
        videoAVP->init();
        std::shared_ptr<ViroContext> context = context_w.lock();
        if (!context) {
            return;
        }
        videoAVP->bindSurface(std::dynamic_pointer_cast<VRODriverOpenGL>(context->getDriver()));
    });
    videoTexture = videoAVP;
#else
    //TODO wasm
#endif

    return VRO_REF_NEW(VROVideoTexture, videoTexture);
}

VRO_METHOD(VRO_REF(VideoDelegate), nativeCreateVideoDelegate)(VRO_NO_ARGS) {

    std::shared_ptr<VideoDelegate> delegate = std::make_shared<VideoDelegate>(obj);
    return VRO_REF_NEW(VideoDelegate, delegate);
}

VRO_METHOD(void, nativeAttachDelegate)(VRO_ARGS
                                       VRO_REF(VROVideoTexture) textureRef,
                                       VRO_REF(VideoDelegate) delegateRef) {

    std::weak_ptr<VROVideoTexture> videoTexture_w = VRO_REF_GET(VROVideoTexture, textureRef);
    std::weak_ptr<VideoDelegate> videoDelegate_w = VRO_REF_GET(VideoDelegate, delegateRef);

    VROPlatformDispatchAsyncRenderer([videoTexture_w, videoDelegate_w] {
        std::shared_ptr<VROVideoTexture> videoTexture = videoTexture_w.lock();
        if (!videoTexture) {
            return;
        }
        std::shared_ptr<VideoDelegate> videoDelegate = videoDelegate_w.lock();
        if (!videoDelegate) {
            return;
        }

        videoTexture->setDelegate(videoDelegate);
        videoDelegate->onReady();
    });
}

VRO_METHOD(void, nativeDeleteVideoTexture)(VRO_ARGS
                                           VRO_REF(VROVideoTexture) textureRef) {
    std::shared_ptr<VROVideoTexture> videoTexture = VRO_REF_GET(VROVideoTexture, textureRef);
    videoTexture->pause();
    VRO_REF_DELETE(VROVideoTexture, textureRef);
}

VRO_METHOD(void, nativeDeleteVideoDelegate)(VRO_ARGS
                                            VRO_REF(VideoDelegate) delegateRef) {
    VRO_REF_DELETE(VideoDelegate, delegateRef);
}

VRO_METHOD(void, nativePause)(VRO_ARGS
                              VRO_REF(VROVideoTexture) textureRef) {
    std::weak_ptr<VROVideoTexture> videoTexture_w = VRO_REF_GET(VROVideoTexture, textureRef);
    VROPlatformDispatchAsyncRenderer([videoTexture_w] {
        std::shared_ptr<VROVideoTexture> videoTexture = videoTexture_w.lock();
        if (!videoTexture) {
            return;
        }
        videoTexture->pause();
    });
}

VRO_METHOD(void, nativePlay)(VRO_ARGS
                             VRO_REF(VROVideoTexture) textureRef) {

    std::weak_ptr<VROVideoTexture> videoTexture_w = VRO_REF_GET(VROVideoTexture, textureRef);
    VROPlatformDispatchAsyncRenderer([videoTexture_w] {
        std::shared_ptr<VROVideoTexture> videoTexture = videoTexture_w.lock();
        if (!videoTexture) {
            return;
        }
        videoTexture->play();
    });
}

VRO_METHOD(void, nativeSetMuted)(VRO_ARGS
                                 VRO_REF(VROVideoTexture) textureRef,
                                 VRO_BOOL muted) {

    std::weak_ptr<VROVideoTexture> videoTexture_w = VRO_REF_GET(VROVideoTexture, textureRef);
    VROPlatformDispatchAsyncRenderer([videoTexture_w, muted] {
        std::shared_ptr<VROVideoTexture> videoTexture = videoTexture_w.lock();
        if (!videoTexture) {
            return;
        }
        videoTexture->setMuted(muted);
    });
}

VRO_METHOD(void, nativeSetVolume)(VRO_ARGS
                                  VRO_REF(VROVideoTexture) textureRef,
                                  VRO_FLOAT volume) {

    std::weak_ptr<VROVideoTexture> videoTexture_w = VRO_REF_GET(VROVideoTexture, textureRef);
    VROPlatformDispatchAsyncRenderer([videoTexture_w, volume] {
        std::shared_ptr<VROVideoTexture> videoTexture = videoTexture_w.lock();
        if (!videoTexture) {
            return;
        }
        videoTexture->setVolume(volume);
    });
}

VRO_METHOD(void, nativeSetLoop)(VRO_ARGS
                                VRO_REF(VROVideoTexture) textureRef,
                                VRO_BOOL loop) {

    std::weak_ptr<VROVideoTexture> videoTexture_w = VRO_REF_GET(VROVideoTexture, textureRef);
    VROPlatformDispatchAsyncRenderer([videoTexture_w, loop] {
        std::shared_ptr<VROVideoTexture> videoTexture = videoTexture_w.lock();
        if (!videoTexture) {
            return;
        }
        videoTexture->setLoop(loop);
    });
}

VRO_METHOD(void, nativeSeekToTime)(VRO_ARGS
                                   VRO_REF(VROVideoTexture) textureRef,
                                   VRO_FLOAT seconds) {

    std::weak_ptr<VROVideoTexture> videoTexture_w = VRO_REF_GET(VROVideoTexture, textureRef);
    VROPlatformDispatchAsyncRenderer([videoTexture_w, seconds] {
        std::shared_ptr<VROVideoTexture> videoTexture = videoTexture_w.lock();
        if (!videoTexture) {
            return;
        }
        videoTexture->seekToTime(seconds);
    });
}

VRO_METHOD(void, nativeLoadSource)(VRO_ARGS
                                   VRO_REF(VROVideoTexture) textureRef,
                                   VRO_STRING source,
                                   VRO_REF(ViroContext) context_j) {
    VRO_METHOD_PREAMBLE;

    // Grab required objects from the RenderContext required for initialization
    std::string strVideoSource = VRO_STRING_STL(source);
    std::weak_ptr<VROVideoTexture> videoTexture_w = VRO_REF_GET(VROVideoTexture, textureRef);
    std::weak_ptr<ViroContext> context_w = VRO_REF_GET(ViroContext, context_j);

    VROPlatformDispatchAsyncRenderer([videoTexture_w, context_w, strVideoSource] {
        std::shared_ptr<VROVideoTexture> videoTexture = videoTexture_w.lock();
        if (!videoTexture) {
            return;
        }
        std::shared_ptr<ViroContext> context = context_w.lock();
        if (!context) {
            return;
        }

        std::shared_ptr<VROFrameSynchronizer> frameSynchronizer = context->getFrameSynchronizer();
        std::shared_ptr<VRODriver> driver = context->getDriver();
        videoTexture->loadVideo(strVideoSource, frameSynchronizer, driver);
        videoTexture->prewarm();
    });
}

}  // extern "C"