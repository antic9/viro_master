//
//  VROVideoTextureiOS.h
//  ViroRenderer
//
//  Created by Raj Advani on 11/10/16.
//  Copyright © 2016 Viro Media. All rights reserved.
//

#ifndef VROVideoTextureiOS_h
#define VROVideoTextureiOS_h

#import "VROVideoTexture.h"
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CVMetalTextureCache.h>
#import <memory>

@class VROVideoCaptureDelegate;
@class VROAVPlayerDelegate;
@class VROVideoNotificationListener;
class VRORenderContext;
class VROFrameSynchronizer;
class VRODriver;
class VROMaterial;
class VROVideoTextureCache;

static const long kInFlightVideoTextures = 3;

class VROVideoTextureiOS : public VROVideoTexture {
    
public:
    
    VROVideoTextureiOS(VROStereoMode stereoMode = VROStereoMode::None);
    virtual ~VROVideoTextureiOS();
    
    /*
     Delete any rendering resources. Invoked prior to destruction, on the
     rendering thread.
     */
    virtual void deleteGL();
    
    /*
     Use this video texture to display the contents of the given URL. The video
     will not run until play() is invoked.
     */
    void loadVideo(std::string url,
                   std::shared_ptr<VROFrameSynchronizer> frameSynchronizer,
                   std::shared_ptr<VRODriver> driver);
    
    /*
     Perform video initialization (which causes a stutter) early.
     */
    void prewarm();

    void playerWillBuffer();
    void playerDidBuffer();

    void onFrameWillRender(const VRORenderContext &context);
    void onFrameDidRender(const VRORenderContext &context);
    
    void pause();
    void play();
    bool isPaused();
    
    void seekToTime(float seconds);
    float getCurrentTimeInSeconds();
    float getVideoDurationInSeconds();

    void setMuted(bool muted);
    void setVolume(float volume);
    void setLoop(bool loop);
    
    void setDelegate(std::shared_ptr<VROVideoDelegateInternal> delegate);
    
    /*
     Internal: invoked by the delegate.
     */
    void displayPixelBuffer(std::unique_ptr<VROTextureSubstrate> substrate);
    
private:
    
    /*
     AVPlayer for recorded video playback.
     */
    AVPlayer *_player;
    bool _paused;
    bool _loop;
    VROAVPlayerDelegate *_avPlayerDelegate;
    VROVideoNotificationListener *_videoNotificationListener;
    
};

/*
 Delegate for receiving video output from URLs.
 */
@interface VROAVPlayerDelegate : NSObject <AVPlayerItemOutputPullDelegate>

- (id)initWithVideoTexture:(VROVideoTextureiOS *)texture player:(AVPlayer *)player
                    driver:(std::shared_ptr<VRODriver>)driver;
- (void)renderFrame;

@end

/*
 Simple object to register for AVPlayer notifications
 */
@interface VROVideoNotificationListener : NSObject

- (id)initWithVideoPlayer:(AVPlayer *)player loop:(BOOL)loop
            videoDelegate:(std::shared_ptr<VROVideoDelegateInternal>)videoDelegate; 

- (void)shouldLoop:(BOOL)loop;
- (void)setDelegate:(std::shared_ptr<VROVideoDelegateInternal>)videoDelegate;

@end

#endif /* VROVideoTextureiOS_h */
