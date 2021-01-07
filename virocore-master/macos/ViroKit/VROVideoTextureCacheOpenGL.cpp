//
//  VROVideoTextureCacheOpenGL.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 5/19/16.
//  Copyright © 2016 Viro Media. All rights reserved.
//

#include "VROVideoTextureCacheOpenGL.h"
#include "VROLog.h"
#include "VROTextureSubstrateOpenGL.h"

VROVideoTextureCacheOpenGL::VROVideoTextureCacheOpenGL(CGLContextObj cglContext, CGLPixelFormatObj pixelFormat,
                                                       std::shared_ptr<VRODriverOpenGL> driver)
    : _currentTextureIndex(0),
      _driver(driver) {
    CVReturn textureCacheError = CVOpenGLTextureCacheCreate(kCFAllocatorDefault, NULL, cglContext,
                                                            pixelFormat, NULL, &_cache);
    if (textureCacheError) {
        pinfo("ERROR: Couldnt create a texture cache");
        pabort();
    }
    for (int i = 0; i < kVideoTextureCacheOpenGLNumTextures; i++) {
        _textureRef[i] = NULL;
    }
    ALLOCATION_TRACKER_ADD(VideoTextureCaches, 1);
}

VROVideoTextureCacheOpenGL::~VROVideoTextureCacheOpenGL() {
    for (int i = 0; i < kVideoTextureCacheOpenGLNumTextures; i++) {
        if (_textureRef[i] != NULL) {
            CVBufferRelease(_textureRef[i]);
        }
    }
    CFRelease(_cache);
    ALLOCATION_TRACKER_SUB(VideoTextureCaches, 1);
}

std::unique_ptr<VROTextureSubstrate> VROVideoTextureCacheOpenGL::createTextureSubstrate(CMSampleBufferRef sampleBuffer, bool sRGB) {
    CVBufferRelease(_textureRef[_currentTextureIndex]);
    _textureRef[_currentTextureIndex] = NULL;
    
    _currentTextureIndex = (_currentTextureIndex + 1) % kVideoTextureCacheOpenGLNumTextures;
    CVOpenGLTextureCacheFlush(_cache, 0);

    CVImageBufferRef sourceImageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    int width  = (int) CVPixelBufferGetWidth(sourceImageBuffer);
    int height = (int) CVPixelBufferGetHeight(sourceImageBuffer);
    
    CVReturn error;
    error = CVOpenGLTextureCacheCreateTextureFromImage(kCFAllocatorDefault, _cache, sourceImageBuffer,
                                                         NULL, &_textureRef[_currentTextureIndex]);
    if (error) {
        pabort("Failed to create texture from image");
    }
    
    GLuint texture = CVOpenGLTextureGetName(_textureRef[_currentTextureIndex]);
    if (texture == 0) {
        pabort("Failed to retreive texture from texture ref");
    }
    
    return std::unique_ptr<VROTextureSubstrateOpenGL>(new VROTextureSubstrateOpenGL(GL_TEXTURE_2D, texture, _driver, false));
}

std::unique_ptr<VROTextureSubstrate> VROVideoTextureCacheOpenGL::createTextureSubstrate(CVPixelBufferRef pixelBuffer, bool sRGB) {
    CVBufferRelease(_textureRef[_currentTextureIndex]);
    _textureRef[_currentTextureIndex] = NULL;
    
    _currentTextureIndex = (_currentTextureIndex + 1) % kVideoTextureCacheOpenGLNumTextures;
    CVOpenGLTextureCacheFlush(_cache, 0);
    
    int width  = (int) CVPixelBufferGetWidth(pixelBuffer);
    int height = (int) CVPixelBufferGetHeight(pixelBuffer);
    
    CVReturn error;
    error = CVOpenGLTextureCacheCreateTextureFromImage(kCFAllocatorDefault, _cache, pixelBuffer,
                                                       NULL, &_textureRef[_currentTextureIndex]);
    if (error) {
        pabort("Failed to create texture from image");
    }
    GLuint texture = CVOpenGLTextureGetName(_textureRef[_currentTextureIndex]);
    if (texture == 0) {
        pabort("Failed to retreive texture from texture ref");
    }
    
    return std::unique_ptr<VROTextureSubstrateOpenGL>(new VROTextureSubstrateOpenGL(GL_TEXTURE_2D, texture, _driver, false));
}

std::vector<std::unique_ptr<VROTextureSubstrate>> VROVideoTextureCacheOpenGL::createYCbCrTextureSubstrates(CVPixelBufferRef pixelBuffer) {
    pabort("YCbCr not supported on OSX");
}
