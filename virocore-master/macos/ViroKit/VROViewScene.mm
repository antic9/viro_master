//
//  VROViewScene.m
//  ViroRenderer
//
//  Created by Raj Advani on 3/1/18.
//  Copyright © 2018 Viro Media. All rights reserved.
//

#import "VROViewScene.h"
#import "VRORenderer.h"
#import "VROSceneController.h"
#import "VRORenderDelegateiOS.h"
#import "VROTime.h"
#import "VROEye.h"
#import "VRODriverOpenGLMacOS.h"
#import "VROConvert.h"
#import "VRONodeCamera.h"
#import "VROChoreographer.h"
#import "VROInputControllerAR.h"
#import <QuartzCore/CVDisplayLink.h>

static VROVector3f const kZeroVector = VROVector3f();

@interface VROViewScene () {
    std::shared_ptr<VRORenderer> _renderer;
    std::shared_ptr<VROSceneController> _sceneController;
    std::shared_ptr<VRORenderDelegateiOS> _renderDelegateWrapper;
    std::shared_ptr<VRODriverOpenGL> _driver;
    std::shared_ptr<VROInputControllerAR> _inputController;
    VRORendererConfiguration _rendererConfig;
    CVDisplayLinkRef _displayLink;
    
    NSOpenGLContext *_openGLContext;
    NSOpenGLPixelFormat *_pixelFormat;
    
    std::recursive_mutex _taskQueueMutex;
    std::queue<std::function<void()>> _taskQueue;
    
    int _frame;
    double _suspendedNotificationTime;
}

@end

@implementation VROViewScene

@dynamic renderDelegate;
@dynamic sceneController;

#pragma mark - Initialization

static CVReturn VRODisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *now, const CVTimeStamp *outputTime,
                                      CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext) {
    [(__bridge VROViewScene *)displayLinkContext drawFrame];
    return kCVReturnSuccess;
}

- (id)initWithFrame:(NSRect)frameRect
             config:(VRORendererConfiguration)config
       shareContext:(NSOpenGLContext *)context {
    
    NSOpenGLPixelFormatAttribute attribs[] = {
        kCGLPFAAccelerated,
        kCGLPFANoRecovery,
        kCGLPFADoubleBuffer,
        kCGLPFAColorSize, 24,
        kCGLPFADepthSize, 16,
        NSOpenGLPFAMultisample,
        NSOpenGLPFASampleBuffers, (NSOpenGLPixelFormatAttribute) 1,
        NSOpenGLPFASamples, (NSOpenGLPixelFormatAttribute) 4,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        0
    };
    
    _rendererConfig = config;
    _pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
    if (!_pixelFormat) {
        pabort("No OpenGL pixel format");
    }
    _openGLContext = [[NSOpenGLContext alloc] initWithFormat:_pixelFormat shareContext:context];
    GLint swapInt = 1;
    [_openGLContext setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
    
    NSLog(@"Created new OpenGL context %p", _openGLContext);
    if (self = [super initWithFrame:frameRect]) {
        [_openGLContext makeCurrentContext];
        
        // Synchronize buffer swaps with vertical refresh rate
        GLint swapInt = 1;
        [_openGLContext setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
        
        // Create a display link capable of being used with all active displays
        CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
        
        // Set the renderer output callback function
        CVDisplayLinkSetOutputCallback(_displayLink, &VRODisplayLinkCallback, (__bridge void *)self);
        
        // Set the display link for the current renderer
        CGLContextObj cglContext = [_openGLContext CGLContextObj];
        CGLPixelFormatObj cglPixelFormat = [_pixelFormat CGLPixelFormatObj];
        CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(_displayLink, cglContext, cglPixelFormat);
        
        // Activate the display link
        CVDisplayLinkStart(_displayLink);
    }
    return self;
}

- (void)initRenderer:(VRORendererConfiguration)config {
    VROPlatformSetType(VROPlatformType::iOSSceneView);
    VROPlatformSetOpenGLContext(_openGLContext, self);
    VROThreadRestricted::setThread(VROThreadName::Renderer);
    
    _driver = std::make_shared<VRODriverOpenGLMacOS>(_openGLContext, _pixelFormat);
    _frame = 0;
    _suspendedNotificationTime = VROTimeCurrentSeconds();
    _inputController = std::make_shared<VROInputControllerAR>(self.frame.size.width,
                                                              self.frame.size.height,
                                                              _driver);
    _renderer = std::make_shared<VRORenderer>(config, _inputController);
    _renderer->setDelegate(_renderDelegateWrapper);
    
    glEnable(GL_FRAMEBUFFER_SRGB);
}

- (void)surfaceNeedsUpdate:(NSNotification *)notification {
    [self update];
}

- (void)lockFocus {
    [super lockFocus];
    if ([_openGLContext view] != self) {
        [_openGLContext setView:self];
    }
}

- (void)clearGLContext {
    
}

- (void)drawFrame {
    [_openGLContext makeCurrentContext];
    
    [self processTasks];
    if (_frame == 0) {
        [self initRenderer:_rendererConfig];
    }

    @autoreleasepool {
        [self renderFrame];
    }
    ++_frame;
    ALLOCATION_TRACKER_PRINT();
    [_openGLContext flushBuffer];
}

- (void)processTasks {
    std::lock_guard<std::recursive_mutex> lock(_taskQueueMutex);
    while (!_taskQueue.empty()) {
        std::function<void()> task = _taskQueue.front();
        _taskQueue.pop();
        task();
    }
}

- (void)queueRendererTask:(std::function<void()>)task {
    std::lock_guard<std::recursive_mutex> lock(_taskQueueMutex);
    _taskQueue.push(task);
}

- (BOOL)acceptsFirstResponder {
    // We want this view to be able to receive key events
    return YES;
}

- (void)keyDown:(NSEvent *)theEvent {
    
}

- (void)mouseDown:(NSEvent *)theEvent {
    
}

- (void)startAnimation {
    if (_displayLink && !CVDisplayLinkIsRunning(_displayLink)) {
        CVDisplayLinkStart(_displayLink);
    }
}

- (void)stopAnimation {
    if (_displayLink && CVDisplayLinkIsRunning(_displayLink)) {
        CVDisplayLinkStop(_displayLink);
    }
}

- (void)dealloc {
    VROThreadRestricted::unsetThread();

    // Stop and release the display link
    CVDisplayLinkStop(_displayLink);
    CVDisplayLinkRelease(_displayLink);
    
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:NSViewGlobalFrameDidChangeNotification
                                                  object:self];
}

- (void)update {
    
}

- (void)reshape {
    [_openGLContext update];
    if (_inputController) {
        _inputController->setViewportSize(self.frame.size.width,
                                          self.frame.size.height);
    }
}

- (void)setBackgroundColor:(NSColor *)color {
    CGFloat r, g, b, a;
    [color getRed:&r green:&g blue:&b alpha:&a];
    VROVector4f color_v((float) r, (float) g, (float) b, (float) a);
    
    self.renderer->setClearColor(color_v, _driver);
    self.layer.opaque = (a > 0.999);
}

- (void)deleteGL {
    if (_sceneController) {
        _sceneController->getScene()->getRootNode()->deleteGL();
    }
}

- (void)setPaused:(BOOL)paused {
    if (paused) {
        CVDisplayLinkStop(_displayLink);
    } else {
        CVDisplayLinkStart(_displayLink);
    }
}

- (BOOL)setShadowsEnabled:(BOOL)enabled {
    return _renderer->getChoreographer()->setShadowsEnabled(enabled);
}

- (BOOL)setHDREnabled:(BOOL)enabled {
    return _renderer->getChoreographer()->setHDREnabled(enabled);
}

- (BOOL)setPBREnabled:(BOOL)enabled {
    return _renderer->getChoreographer()->setPBREnabled(enabled);
}

- (BOOL)setBloomEnabled:(BOOL)enabled {
    return _renderer->getChoreographer()->setBloomEnabled(enabled);
}

#pragma mark - Settings and Notifications

- (void)orientationDidChange:(NSNotification *)notification {
   
}

- (void)applicationWillResignActive:(NSNotification *)notification {
    CVDisplayLinkStop(_displayLink);
}

- (void)applicationDidBecomeActive:(NSNotification *)notification {
    CVDisplayLinkStart(_displayLink);
}

- (void)setRenderDelegate:(id<VRORenderDelegate>)renderDelegate {
    _renderDelegateWrapper = std::make_shared<VRORenderDelegateiOS>(renderDelegate);
    [self startAnimation];
}

- (void)setDebugHUDEnabled:(BOOL)enabled {
    _renderer->setDebugHUDEnabled(enabled);
}

#pragma mark - Camera

- (void)setPointOfView:(std::shared_ptr<VRONode>)node {
    _renderer->setPointOfView(node);
}

#pragma mark - Scene Loading

- (void)setSceneController:(std::shared_ptr<VROSceneController>)sceneController {
    [self setSceneController:sceneController duration:0 timingFunction:VROTimingFunctionType::EaseIn];
}

- (void)setSceneController:(std::shared_ptr<VROSceneController>)sceneController
                  duration:(float)seconds
            timingFunction:(VROTimingFunctionType)timingFunctionType {
    _sceneController = sceneController;
    _renderer->setSceneController(sceneController, seconds, timingFunctionType, _driver);
}

#pragma mark - Getters

- (std::shared_ptr<VROFrameSynchronizer>)frameSynchronizer {
    return _renderer->getFrameSynchronizer();
}

- (std::shared_ptr<VRORenderer>)renderer {
    return _renderer;
}

- (std::shared_ptr<VROChoreographer>)choreographer {
    return _renderer->getChoreographer();
}

#pragma mark - Rendering

- (void)renderFrame {
    glEnable(GL_DEPTH_TEST);    
    _driver->setCullMode(VROCullMode::Back);
    
    VROViewport viewport(0, 0, self.bounds.size.width, self.bounds.size.height);

    /*
     The viewport can be 0, if say in React Native, the user accidentally messes up their
     styles and React Native lays the view out with 0 width or height. No use rendering
     in this case.
     */
    if (viewport.getWidth() == 0 || viewport.getHeight() == 0) {
        return;
    }
    
    VROFieldOfView fov = _renderer->computeUserFieldOfView(viewport.getWidth(), viewport.getHeight());
    VROMatrix4f projection = fov.toPerspectiveProjection(kZNear, _renderer->getFarClippingPlane());
    
    _renderer->prepareFrame(_frame, viewport, fov, VROMatrix4f::identity(), projection, _driver);
    glViewport(viewport.getX(), viewport.getY(), viewport.getWidth(), viewport.getHeight());
    _renderer->renderEye(VROEyeType::Monocular, _renderer->getLookAtMatrix(), projection, viewport, _driver);
    _renderer->renderHUD(VROEyeType::Monocular, VROMatrix4f::identity(), projection, _driver);
    _renderer->endFrame(_driver);
}

- (void)recenterTracking {
    // No-op in Scene view
}

@end
