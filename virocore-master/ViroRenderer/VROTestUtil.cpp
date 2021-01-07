//
//  VROTestUtil.cpp
//  ViroKit
//
//  Created by Raj Advani on 10/1/17.
//  Copyright © 2017 Viro Media. All rights reserved.
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

#include "VROTestUtil.h"
#include "VRODefines.h"
#include "VROTexture.h"
#include "VRONode.h"
#include "VROVector3f.h"
#include "VROGeometry.h"
#include "VROFBXLoader.h"
#include "VROExecutableAnimation.h"
#include "VRODriverOpenGL.h"
#include "VROTextureUtil.h"
#include "VROCompress.h"
#include "VROModelIOUtil.h"
#include "VROHDRLoader.h"
#include "VROGLTFLoader.h"

#if VRO_PLATFORM_IOS
#include <UIKit/UIKit.h>
#include "VROImageiOS.h"
#include "VROVideoTextureiOS.h"
#endif

#if VRO_PLATFORM_MACOS
#include <AppKit/AppKit.h>
#include "VROImageMacOS.h"
#endif

#if VRO_PLATFORM_ANDROID
#include "VROPlatformUtil.h"
#include "VROImageAndroid.h"
#include "VROVideoTextureAVP.h"
#endif

std::string VROTestUtil::getURLForResource(std::string resource, std::string type) {
#if VRO_PLATFORM_IOS || VRO_PLATFORM_MACOS
    NSString *objPath = [[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:resource.c_str()]
                                                        ofType:[NSString stringWithUTF8String:type.c_str()]];
    NSURL *objURL = [NSURL fileURLWithPath:objPath];
    return std::string([[objURL description] UTF8String]);
#elif VRO_PLATFORM_ANDROID
    return "file:///android_asset/" + resource  + "." + type;
#endif
}

void *VROTestUtil::loadDataForResource(std::string resource, std::string type, int *outLength) {
#if VRO_PLATFORM_IOS || VRO_PLATFORM_MACOS
    NSString *path = [[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:resource.c_str()]
                                                        ofType:[NSString stringWithUTF8String:type.c_str()]];
    NSURL *url = [NSURL fileURLWithPath:path];
    NSData *data = [NSData dataWithContentsOfURL:url];
    
    void *outBytes = malloc([data length]);
    memcpy(outBytes, [data bytes], [data length]);
    
    *outLength = (int)[data length];
    return outBytes;
#elif VRO_PLATFORM_ANDROID
    std::string path = VROPlatformCopyAssetToFile(resource + "." + type);
    return VROPlatformLoadFile(path, outLength);
#endif
}

std::shared_ptr<VROTexture> VROTestUtil::loadCloudBackground() {
    VROTextureInternalFormat format = VROTextureInternalFormat::RGBA8;
    
#if VRO_PLATFORM_IOS
    std::vector<std::shared_ptr<VROImage>> cubeImages =  {
        std::make_shared<VROImageiOS>([UIImage imageNamed:@"px1.jpg"], format),
        std::make_shared<VROImageiOS>([UIImage imageNamed:@"nx1.jpg"], format),
        std::make_shared<VROImageiOS>([UIImage imageNamed:@"py1.jpg"], format),
        std::make_shared<VROImageiOS>([UIImage imageNamed:@"ny1.jpg"], format),
        std::make_shared<VROImageiOS>([UIImage imageNamed:@"pz1.jpg"], format),
        std::make_shared<VROImageiOS>([UIImage imageNamed:@"nz1.jpg"], format)
    };
    
    return std::make_shared<VROTexture>(true, cubeImages);
#elif VRO_PLATFORM_ANDROID
    std::vector<std::shared_ptr<VROImage>> cubeImages = {
            std::make_shared<VROImageAndroid>("px1.jpg", format),
            std::make_shared<VROImageAndroid>("nx1.jpg", format),
            std::make_shared<VROImageAndroid>("py1.jpg", format),
            std::make_shared<VROImageAndroid>("ny1.jpg", format),
            std::make_shared<VROImageAndroid>("pz1.jpg", format),
            std::make_shared<VROImageAndroid>("nz1.jpg", format)
    };

    return std::make_shared<VROTexture>(true, cubeImages);
#elif VRO_PLATFORM_WASM
    
    
#elif VRO_PLATFORM_MACOS
    return nullptr;
#endif
}

std::shared_ptr<VROTexture> VROTestUtil::loadNiagaraBackground() {
    VROTextureInternalFormat format = VROTextureInternalFormat::RGBA8;
    
#if VRO_PLATFORM_IOS
    std::vector<std::shared_ptr<VROImage>> cubeImages =  {
        std::make_shared<VROImageiOS>([UIImage imageNamed:@"px"], format),
        std::make_shared<VROImageiOS>([UIImage imageNamed:@"nx"], format),
        std::make_shared<VROImageiOS>([UIImage imageNamed:@"py"], format),
        std::make_shared<VROImageiOS>([UIImage imageNamed:@"ny"], format),
        std::make_shared<VROImageiOS>([UIImage imageNamed:@"pz"], format),
        std::make_shared<VROImageiOS>([UIImage imageNamed:@"nz"], format)
    };
    
    return std::make_shared<VROTexture>(true, cubeImages);
#elif VRO_PLATFORM_ANDROID
    std::vector<std::shared_ptr<VROImage>> cubeImages = {
        std::make_shared<VROImageAndroid>("px.png", format),
        std::make_shared<VROImageAndroid>("nx.png", format),
        std::make_shared<VROImageAndroid>("py.png", format),
        std::make_shared<VROImageAndroid>("ny.png", format),
        std::make_shared<VROImageAndroid>("pz.png", format),
        std::make_shared<VROImageAndroid>("nz.png", format)
    };

    return std::make_shared<VROTexture>(true, cubeImages);
#elif VRO_PLATFORM_WASM
    
    
#elif VRO_PLATFORM_MACOS
    return nullptr;
#endif
}

std::shared_ptr<VROTexture> VROTestUtil::loadWestlakeBackground() {
#if VRO_PLATFORM_IOS
    return std::make_shared<VROTexture>(true, VROMipmapMode::None,
                                        std::make_shared<VROImageiOS>([UIImage imageNamed:@"360_westlake.jpg"],
                                                                      VROTextureInternalFormat::RGBA8));
#elif VRO_PLATFORM_ANDROID
    return std::make_shared<VROTexture>(true, VROMipmapMode::None,
                                        std::make_shared<VROImageAndroid>("360_westlake.jpg",
                                                                          VROTextureInternalFormat::RGBA8));
#elif VRO_PLATFORM_WASM
    
    
#elif VRO_PLATFORM_MACOS
    return nullptr;
#endif
}

std::shared_ptr<VROTexture> VROTestUtil::loadRadianceHDRTexture(std::string texture) {
    std::string path;
#if VRO_PLATFORM_IOS || VRO_PLATFORM_MACOS
    NSString *fbxPath = [[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:texture.c_str()]
                                                        ofType:@"hdr"];
    path = std::string([fbxPath UTF8String]);
#elif VRO_PLATFORM_ANDROID
    path = VROPlatformCopyAssetToFile(texture + ".hdr");
#elif VRO_PLATFORM_WASM
    path = "/" + texture + ".hdr";
#endif
    return VROHDRLoader::loadRadianceHDRTexture(path);
}

std::shared_ptr<VROTexture> VROTestUtil::loadHDRTexture(std::string texture) {
    int fileLength;
    void *fileData = VROTestUtil::loadDataForResource(texture, "vhd", &fileLength);
    
    std::string data_gzip((char *)fileData, fileLength);
    std::string data_texture = VROCompress::decompress(data_gzip);
    
    VROTextureFormat format;
    int texWidth;
    int texHeight;
    std::vector<uint32_t> mipSizes;
    std::shared_ptr<VROData> texData = VROTextureUtil::readVHDHeader(data_texture,
                                                                     &format, &texWidth, &texHeight, &mipSizes);
    std::vector<std::shared_ptr<VROData>> dataVec = { texData };
    
    free (fileData);
    return std::make_shared<VROTexture>(VROTextureType::Texture2D,
                                        format,
                                        VROTextureInternalFormat::RGB9_E5, true,
                                        VROMipmapMode::None,
                                        dataVec, texWidth, texHeight, mipSizes);
}

std::shared_ptr<VROTexture> VROTestUtil::loadDiffuseTexture(std::string texture, VROMipmapMode mipmap, VROStereoMode stereo) {
    VROTextureInternalFormat format = VROTextureInternalFormat::RGBA8;

#if VRO_PLATFORM_IOS
    return std::make_shared<VROTexture>(true, mipmap,
                                        std::make_shared<VROImageiOS>([UIImage imageNamed:[NSString stringWithUTF8String:texture.c_str()]], format), stereo);
#elif VRO_PLATFORM_ANDROID
    if (texture.find(".") == std::string::npos) {
        texture = texture + ".png";
    }
    return std::make_shared<VROTexture>(true, mipmap,
                                        std::make_shared<VROImageAndroid>(texture.c_str(), format), stereo);
#elif VRO_PLATFORM_WASM
    
    
#elif VRO_PLATFORM_MACOS
    return std::make_shared<VROTexture>(true, mipmap,
                                        std::make_shared<VROImageMacOS>([NSImage imageNamed:[NSString stringWithUTF8String:texture.c_str()]], format), stereo);
#endif
}

std::shared_ptr<VROTexture> VROTestUtil::loadSpecularTexture(std::string texture) {
    return loadTexture(texture, true);
}

std::shared_ptr<VROTexture> VROTestUtil::loadNormalTexture(std::string texture) {
    return loadTexture(texture, false);
}

std::shared_ptr<VROTexture> VROTestUtil::loadTexture(std::string texture, bool sRGB) {
    VROTextureInternalFormat format = VROTextureInternalFormat::RGBA8;
    
#if VRO_PLATFORM_IOS
    return std::make_shared<VROTexture>(sRGB, VROMipmapMode::Runtime,
                                        std::make_shared<VROImageiOS>([UIImage imageNamed:[NSString stringWithUTF8String:texture.c_str()]], format));
#elif VRO_PLATFORM_ANDROID
    if (texture.find(".") == std::string::npos) {
        texture = texture + ".png";
    }
    return std::make_shared<VROTexture>(sRGB, VROMipmapMode::Runtime,
                                        std::make_shared<VROImageAndroid>(texture.c_str(), format));
#elif VRO_PLATFORM_WASM
    
    
#elif VRO_PLATFORM_MACOS
    return nullptr;
#endif
}

std::shared_ptr<VRONode> VROTestUtil::loadFBXModel(std::string model, VROVector3f position, VROVector3f scale, VROVector3f rotation,
                                                   int lightMask, std::string animation, std::shared_ptr<VRODriver> driver,
                                                   std::function<void(std::shared_ptr<VRONode>, bool)> onFinish) {
    std::string url;
    std::string base;
    VROResourceType resourceType = VROResourceType::URL;
    
#if VRO_PLATFORM_IOS || VRO_PLATFORM_MACOS
    NSString *fbxPath = [[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:model.c_str()]
                                                        ofType:@"vrx"];
    NSURL *fbxURL = [NSURL fileURLWithPath:fbxPath];
    url = std::string([[fbxURL description] UTF8String]);
    
    NSString *basePath = [fbxPath stringByDeletingLastPathComponent];
    NSURL *baseURL = [NSURL fileURLWithPath:basePath];
    base = std::string([[baseURL description] UTF8String]);
#elif VRO_PLATFORM_ANDROID
    url = "file:///android_asset/" + model + ".vrx";
    base = url.substr(0, url.find_last_of('/'));
#else
    url = "/" + model + ".vrx";
    base = url.substr(0, url.find_last_of('/'));
    resourceType = VROResourceType::LocalFile;
#endif

    std::shared_ptr<VRONode> node = std::make_shared<VRONode>();
    VROFBXLoader::loadFBXFromResource(url, resourceType, node, driver,
                                        [scale, position, rotation, lightMask, animation, onFinish](std::shared_ptr<VRONode> node, bool success) {
                                            if (!success) {
                                                return;
                                            }
                                            
                                            node->setScale(scale);
                                            node->setPosition(position);
                                            node->setRotation(rotation);
                                            setLightMasks(node, lightMask);
                                            
                                            if (node->getGeometry()) {
                                                node->getGeometry()->setName("FBX Root Geometry");
                                            }
                                            for (std::shared_ptr<VRONode> &child : node->getChildNodes()) {
                                                if (child->getGeometry()) {
                                                    child->getGeometry()->setName("FBX Geometry");
                                                }
                                            }
                                            
                                            std::set<std::string> animations = node->getAnimationKeys(true);
                                            for (std::string animation : animations) {
                                                pinfo("Loaded animation [%s]", animation.c_str());
                                            }

                                            if (animations.size() != 0) {
                                                animateTake(node, animation);
                                            }

                                            if (onFinish != nullptr) {
                                                onFinish(node, success);
                                            }
                                            pinfo("FBX HAS LOADED");
                                        });
    return node;
}

std::shared_ptr<VRONode> VROTestUtil::loadGLTFModel(std::string model, std::string ext, VROVector3f position, VROVector3f scale,
                                                   int lightMask, std::string animation, std::shared_ptr<VRODriver> driver,
                                                    std::function<void(std::shared_ptr<VRONode>, bool)> onFinish) {
    std::string url;
    std::string base;
    VROResourceType resourceType = VROResourceType::URL;
    bool isGLBType = VROStringUtil::strcmpinsensitive(ext, "glb");

#if VRO_PLATFORM_IOS || VRO_PLATFORM_MACOS
    NSString *fbxPath = [[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:model.c_str()]
                                                        ofType:[NSString stringWithUTF8String:ext.c_str()]];
    NSURL *fbxURL = [NSURL fileURLWithPath:fbxPath];
    url = std::string([[fbxURL description] UTF8String]);

    NSString *basePath = [fbxPath stringByDeletingLastPathComponent];
    NSURL *baseURL = [NSURL fileURLWithPath:basePath];
    base = std::string([[baseURL description] UTF8String]);
#elif VRO_PLATFORM_ANDROID
    url = "file:///android_asset/" + model + "." + ext;
    base = url.substr(0, url.find_last_of('/'));
#else
    url = "/" + model + ".vrx";
    base = url.substr(0, url.find_last_of('/'));
    resourceType = VROResourceType::LocalFile;
#endif

    std::shared_ptr<VRONode> node = std::make_shared<VRONode>();
    VROGLTFLoader::loadGLTFFromResource(url, {}, resourceType, node, isGLBType, driver,
                                      [scale, position, lightMask, onFinish](std::shared_ptr<VRONode> node, bool success) {
                                          if (!success) {
                                              if (onFinish) {
                                                  onFinish(node, false);
                                              }
                                              return;
                                          }

                                          node->setScale(scale);
                                          node->setPosition(position);
                                          setLightMasks(node, lightMask);

                                          if (node->getGeometry()) {
                                              node->getGeometry()->setName("GLTF Root Geometry");
                                          }
                                          for (std::shared_ptr<VRONode> &child : node->getChildNodes()) {
                                              if (child->getGeometry()) {
                                                  child->getGeometry()->setName("GLTF Geometry");
                                              }
                                          }

                                          if (onFinish) {
                                              onFinish(node, true);
                                          }
                                          pinfo("GLTF HAS LOADED");
                                          pinfo("Bounds %s", node->getUmbrellaBoundingBox().toString().c_str());
                                      });
    return node;
}

void VROTestUtil::animateTake(std::weak_ptr<VRONode> node_w, std::string name) {
    std::shared_ptr<VRONode> node = node_w.lock();
    if (!node) {
        return;
    }

    std::shared_ptr<VROExecutableAnimation> animation = node->getAnimation(name.c_str(), true)->copy();
    animation->setDuration(15);
    animation->execute(node, [node_w, name] {
        animateTake(node_w, name);
    });
}

void VROTestUtil::setLightMasks(std::shared_ptr<VRONode> node, int value) {
    node->setLightReceivingBitMask(value);
    node->setShadowCastingBitMask(value);
    
    for (std::shared_ptr<VRONode> child : node->getChildNodes()) {
        setLightMasks(child, value);
    }
}

std::shared_ptr<VROVideoTexture> VROTestUtil::loadVideoTexture(std::shared_ptr<VRODriver> driver,
                                                               std::function<void(std::shared_ptr<VROVideoTexture>)> callback,
                                                               VROStereoMode stereo) {
#if VRO_PLATFORM_IOS
    std::shared_ptr<VROVideoTexture> texture = std::make_shared<VROVideoTextureiOS>(stereo);
    callback(texture);

    return texture;
#elif VRO_PLATFORM_ANDROID
    std::shared_ptr<VROVideoTextureAVP> videoTexture = std::make_shared<VROVideoTextureAVP>(stereo);
    VROPlatformDispatchAsyncApplication([videoTexture, driver, callback] {
        videoTexture->init();
        VROPlatformDispatchAsyncRenderer([videoTexture, driver, callback] {
            videoTexture->bindSurface(std::dynamic_pointer_cast<VRODriverOpenGL>(driver));
            callback(videoTexture);
        });
    });

    return videoTexture;
#elif VRO_PLATFORM_WASM
    
    
#elif VRO_PLATFORM_MACOS
    return nullptr;
#endif
}

