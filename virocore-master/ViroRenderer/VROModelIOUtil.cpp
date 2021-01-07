//
//  VROModelIOUtil.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 5/2/17.
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

#include "VROModelIOUtil.h"
#include "VROPlatformUtil.h"
#include "VROImage.h"
#include "VROTexture.h"
#include "VROLog.h"
#include "VROTextureUtil.h"
#include "VROStringUtil.h"
#include "VRONode.h"
#include "VROGeometry.h"
#include "VROMaterial.h"

const std::string kAssetURLPrefix = "file:///android_asset";

void VROModelIOUtil::loadTextureAsync(const std::string &name, const std::string &base, VROResourceType type, bool sRGB,
                                      std::shared_ptr<std::map<std::string, std::string>> resourceMap,
                                      std::shared_ptr<std::map<std::string, std::shared_ptr<VROTexture>>> textureCache,
                                      std::function<void(std::shared_ptr<VROTexture> texture)> onFinished) {
    
    // First check the cache, which can only be accessed on the rendering thread
    auto it = textureCache->find(name);
    if (it != textureCache->end()) {
        onFinished(it->second);
        return;
    }

    std::string textureFile;
    if (resourceMap == nullptr) {
        textureFile = base + "/" + name;
    } else {
        textureFile = VROPlatformFindValueInResourceMap(name, *resourceMap);
    }

    retrieveResourceAsync(textureFile, type,
          [name, sRGB, onFinished, textureCache](std::string path, bool isTemp) {
              // Abort (return empty texture) if the file wasn't found
              if (path.length() == 0) {
                  onFinished(nullptr);
                  return;
              }
              
              VROPlatformDispatchAsyncBackground([name, path, sRGB, isTemp, textureCache, onFinished]() {
                  std::shared_ptr<VROTexture> texture = loadLocalTexture(name, path, sRGB, isTemp);

                  VROPlatformDispatchAsyncRenderer([name, texture, textureCache, onFinished]() {
                      if (texture != nullptr) {
                          textureCache->insert(std::make_pair(name, texture));
                      }
                      onFinished(texture);
                  });
              });
              
          },
          [onFinished]() {
              onFinished(nullptr);
          }
    );
}

std::shared_ptr<VROTexture> VROModelIOUtil::loadLocalTexture(std::string name, std::string path, bool sRGB, bool isTemp) {
    std::shared_ptr<VROTexture> texture;
    if (VROStringUtil::endsWith(name, "ktx")) {
        int dataLength;
        void *data = VROPlatformLoadFile(path, &dataLength);
        
        VROTextureFormat format;
        int texWidth;
        int texHeight;
        std::vector<uint32_t> mipSizes;
        std::shared_ptr<VROData> texData = VROTextureUtil::readKTXHeader((uint8_t *) data, (uint32_t) dataLength,
                                                                         &format, &texWidth, &texHeight, &mipSizes);
        std::vector<std::shared_ptr<VROData>> dataVec = { texData };
        
        texture = std::make_shared<VROTexture>(VROTextureType::Texture2D, format,
                                               VROTextureInternalFormat::RGBA8, true,
                                               VROMipmapMode::Pregenerated,
                                               dataVec, texWidth, texHeight, mipSizes);
        return texture;
    }
    else {
        std::shared_ptr<VROImage> image = VROPlatformLoadImageFromFile(path, VROTextureInternalFormat::RGBA8);
        if (isTemp) {
            VROPlatformDeleteFile(path);
        }
        
        if (!image) {
            pinfo("Failed to load texture [%s] at path [%s]", name.c_str(), path.c_str());
            return nullptr;
        }
        else {
            texture = std::make_shared<VROTexture>(sRGB, VROMipmapMode::Runtime, image);
            return texture;
        }
    }
}

void VROModelIOUtil::retrieveResourceAsync(std::string resource, VROResourceType type,
                                           std::function<void(std::string, bool)> onSuccess,
                                           std::function<void()> onFailure) {
    // If we're given a URL with the res:/ prefix, then treat it as a BundledResource
    if (type == VROResourceType::URL && VROStringUtil::startsWith(resource, "res:")) {
        type = VROResourceType::BundledResource;
    }

    if (type == VROResourceType::BundledResource) {
        bool temp;
        std::string path = VROPlatformCopyResourceToFile(resource, &temp);
        onSuccess(path, temp);
    }
    else if (type == VROResourceType::URL) {
        if (!VROStringUtil::startsWith(resource, kAssetURLPrefix)) {
            resource = VROStringUtil::encodeURL(resource);
        }
        VROPlatformDownloadURLToFileAsync(resource, onSuccess, onFailure);
    }
    else {
        onSuccess(resource, false);
    }
}

std::string VROModelIOUtil::retrieveResource(std::string resource, VROResourceType type, bool *isTemp, bool *success) {
    // If we're given a URL with the res:/ prefix, then treat it as a BundledResource
    if (type == VROResourceType::URL && VROStringUtil::startsWith(resource, "res:")) {
        type = VROResourceType::BundledResource;
    }

    std::string path;
    *isTemp = false;
    
    if (type == VROResourceType::BundledResource) {
        path = VROPlatformCopyResourceToFile(resource, isTemp);
        *success = true;
    }
    else if (type == VROResourceType::URL) {
        if (!VROStringUtil::startsWith(resource, kAssetURLPrefix)) {
            resource = VROStringUtil::encodeURL(resource);
        }
        path = VROPlatformDownloadURLToFile(resource, isTemp, success);
    }
    else {
        path = resource;
        *success = true;
    }
    return path;
}

std::shared_ptr<std::map<std::string, std::string>> VROModelIOUtil::createResourceMap(const std::map<std::string, std::string> &resourceMap,
                                                                                      VROResourceType type) {
    std::shared_ptr<std::map<std::string, std::string>> resources = std::make_shared<std::map<std::string, std::string>>();
    if (type == VROResourceType::LocalFile) {
        for (auto &kv : resourceMap) {
            (*resources)[kv.first] = kv.second;
        }
        return resources;
    }
    else if (type == VROResourceType::URL) {
        pabort();
    }
    else {
        for (auto &kv : resourceMap) {
            bool isTemp;
            (*resources)[kv.first] = VROPlatformCopyResourceToFile(kv.second, &isTemp);
        }
        return resources;
    }
};

void VROModelIOUtil::hydrateNodes(std::shared_ptr<VRONode> node, std::shared_ptr<VRODriver> &driver) {
    std::shared_ptr<VROGeometry> geometry = node->getGeometry();
    if (geometry) {
        geometry->prewarm(driver);
        for (const std::shared_ptr<VROMaterial> &material : geometry->getMaterials()) {
            material->prewarm(driver);
        }
    }
    for (std::shared_ptr<VRONode> &node : node->getChildNodes()) {
        hydrateNodes(node, driver);
    }
}

void VROModelIOUtil::hydrateAsync(std::shared_ptr<VRONode> node, std::function<void()> finishedCallback,
                                  std::shared_ptr<VRODriver> &driver) {
    int *unhydratedTextureCount = (int *)malloc(sizeof(int));
    *unhydratedTextureCount = 0;
    
    std::function<void()> callback = [finishedCallback, unhydratedTextureCount]() mutable {
        if (unhydratedTextureCount == nullptr) {
            return;
        }
        
        *unhydratedTextureCount = *unhydratedTextureCount - 1;
        if (*unhydratedTextureCount == 0) {
            free (unhydratedTextureCount);
            unhydratedTextureCount = nullptr;
            
            finishedCallback();
        }
    };
    hydrateAsync(node, callback, unhydratedTextureCount, driver);
    
    // If no textures needed to be hydrated, we invoke the callback immediately
    if (unhydratedTextureCount != nullptr && *unhydratedTextureCount == 0) {
        free (unhydratedTextureCount);
        unhydratedTextureCount = nullptr;
        
        finishedCallback();
    }
}

void VROModelIOUtil::hydrateAsync(std::shared_ptr<VRONode> node, std::function<void()> callback,
                                  int *unhydratedTextureCount,
                                  std::shared_ptr<VRODriver> &driver) {
   
    std::shared_ptr<VROGeometry> geometry = node->getGeometry();
    if (geometry) {
        geometry->prewarm(driver);
        for (const std::shared_ptr<VROMaterial> &material : geometry->getMaterials()) {
            *unhydratedTextureCount += material->hydrateAsync(callback, driver);
        }
    }
    for (std::shared_ptr<VRONode> &node : node->getChildNodes()) {
        hydrateAsync(node, callback, unhydratedTextureCount, driver);
    }
}

