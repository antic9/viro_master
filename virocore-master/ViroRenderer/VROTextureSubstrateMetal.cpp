//
//  VROTextureSubstrateMetal.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 12/4/15.
//  Copyright © 2015 Viro Media. All rights reserved.
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

#include "VROTextureSubstrateMetal.h"
#if VRO_METAL

#include "VROImageUtil.h"
#include "VROTexture.h"
#include "VRODriverMetal.h"
#include "VROLog.h"
#include "VROImage.h"

VROTextureSubstrateMetal::VROTextureSubstrateMetal(VROTextureType type, std::vector<std::shared_ptr<VROImage>> &images,
                                                   std::shared_ptr<VRODriver> &driver) {
    
    VRODriverMetal &metal = (VRODriverMetal &)driver;
    id <MTLDevice> device = metal.getDevice();
    
    if (type == VROTextureType::Texture2D) {
        std::shared_ptr<VROImage> &image = images.front();
        int width = image->getWidth();
        int height = image->getHeight();
        
        size_t dataLength;
        void *data = image->extractRGBA8888(&dataLength);
        
        int bytesPerPixel = 4;
        MTLTextureDescriptor *descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                              width:width
                                                                                             height:height
                                                                                          mipmapped:YES];
        _texture = [device newTextureWithDescriptor:descriptor];
        
        MTLRegion region = MTLRegionMake2D(0, 0, width, height);
        [_texture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytesPerPixel * width];
        
        id <MTLCommandBuffer> textureCommandBuffer = [metal.getCommandQueue() commandBuffer];
        id<MTLBlitCommandEncoder> commandEncoder = [textureCommandBuffer blitCommandEncoder];
        [commandEncoder generateMipmapsForTexture:_texture];
        [commandEncoder endEncoding];
        [textureCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {

        }];
        [textureCommandBuffer commit];
    }
    
    else if (type == VROTextureType::TextureCube && images.size() == 6) {
        passert_msg(images.size() == 6,
                    "Cube texture can only be created from exactly six images");
        
        std::shared_ptr<VROImage> &firstImage = images.front();
        const CGFloat cubeSize = firstImage->getWidth();
        
        const NSUInteger bytesPerPixel = 4;
        const NSUInteger bytesPerRow = bytesPerPixel * cubeSize;
        const NSUInteger bytesPerImage = bytesPerRow * cubeSize;
        
        MTLRegion region = MTLRegionMake2D(0, 0, cubeSize, cubeSize);
        
        MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                                        size:cubeSize
                                                                                                   mipmapped:NO];
        _texture = [device newTextureWithDescriptor:textureDescriptor];
        
        for (size_t slice = 0; slice < 6; ++slice) {
            std::shared_ptr<VROImage> &image = images[slice];
            
            size_t dataLength;
            void *data = image->extractRGBA8888(&dataLength);
            
            passert_msg(image->getWidth() == cubeSize && image->getHeight() == cubeSize,
                        "Cube map images must be square and uniformly-sized");
            
            [_texture replaceRegion:region
                        mipmapLevel:0
                              slice:slice
                          withBytes:data
                        bytesPerRow:bytesPerRow
                      bytesPerImage:bytesPerImage];
        }
    }
    
    else {
        pabort("Invalid texture images received, could not convert to Metal");
    }
    
    ALLOCATION_TRACKER_ADD(TextureSubstrates, 1);
}

VROTextureSubstrateMetal::VROTextureSubstrateMetal(VROTextureType type, VROTextureFormat format,
                                                   std::shared_ptr<VROData> data, int width, int height,
                                                   std::shared_ptr<VRODriver> &driver) {
    
    if (format == VROTextureFormat::ETC2) {
        VRODriverMetal &metal = (VRODriverMetal &)driver;
        id <MTLDevice> device = metal.getDevice();
        
        if (type == VROTextureType::Texture2D) {
            int bytesPerRow = width / 4 * 8; // TODO This is not working
            MTLTextureDescriptor *descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatETC2_RGB8
                                                                                                  width:width
                                                                                                 height:height
                                                                                              mipmapped:NO];
            _texture = [device newTextureWithDescriptor:descriptor];
     
            MTLRegion region = MTLRegionMake2D(0, 0, width, height);
            [_texture replaceRegion:region mipmapLevel:0 withBytes:data->getData() bytesPerRow:bytesPerRow];
        }
        else {
            pabort();
        }
    }
    else if (format == VROTextureFormat::ASTC_4x4_LDR) {
        VRODriverMetal &metal = (VRODriverMetal &)driver;
        id <MTLDevice> device = metal.getDevice();
        
        if (type == VROTextureType::Texture2D) {
            int bytesPerRow = width / 4 * 16; // texels per row / block footprint (4) / * 16 (each ASTC block is 16 bytes)
            MTLTextureDescriptor *descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatASTC_4x4_LDR
                                                                                                  width:width
                                                                                                 height:height
                                                                                              mipmapped:NO];
            _texture = [device newTextureWithDescriptor:descriptor];
            
            MTLRegion region = MTLRegionMake2D(0, 0, width, height);
            [_texture replaceRegion:region mipmapLevel:0 withBytes:data->getData() bytesPerRow:bytesPerRow];
        }
        else {
            pabort();
        }
    }
    else if (format == VROTextureFormat::RGBA8) {
        id <MTLDevice> device = ((VRODriverMetal &)driver).getDevice();
        
        int bytesPerPixel = 4;
        MTLTextureDescriptor *descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                              width:width
                                                                                             height:height
                                                                                          mipmapped:NO];
        
        _texture = [device newTextureWithDescriptor:descriptor];
        
        MTLRegion region = MTLRegionMake2D(0, 0, width, height);
        [_texture replaceRegion:region mipmapLevel:0 withBytes:data->getData() bytesPerRow:bytesPerPixel * width];
    }
    else {
        pabort();
    }
    
    ALLOCATION_TRACKER_ADD(TextureSubstrates, 1);
}

VROTextureSubstrateMetal::~VROTextureSubstrateMetal() {
    ALLOCATION_TRACKER_SUB(TextureSubstrates, 1);
}

#endif

