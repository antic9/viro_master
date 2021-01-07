//
//  VROFBXLoader.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 5/1/17.
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

#include "VROFBXLoader.h"
#include "VRONode.h"
#include "VROPlatformUtil.h"
#include "VROGeometry.h"
#include "VROData.h"
#include "VROStringUtil.h"
#include "VROModelIOUtil.h"
#include "VROSkinner.h"
#include "VROSkeleton.h"
#include "VROBone.h"
#include "VROSkeletalAnimation.h"
#include "VROBoneUBO.h"
#include "VROKeyframeAnimation.h"
#include "VROTaskQueue.h"
#include "Nodes.pb.h"
#include <google/protobuf/io/gzip_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include "VRODefines.h"
#if VRO_PLATFORM_IOS || VRO_PLATFORM_MACOS
#define google google_public
#endif

static bool kDebugFBXLoading = false;

VROGeometrySourceSemantic convert(viro::Node_Geometry_Source_Semantic semantic) {
    switch (semantic) {
        case viro::Node_Geometry_Source_Semantic_Vertex:
            return VROGeometrySourceSemantic::Vertex;
        case viro::Node_Geometry_Source_Semantic_Normal:
            return VROGeometrySourceSemantic::Normal;
        case viro::Node_Geometry_Source_Semantic_Color:
            return VROGeometrySourceSemantic::Color;
        case viro::Node_Geometry_Source_Semantic_Texcoord:
            return VROGeometrySourceSemantic::Texcoord;
        case viro::Node_Geometry_Source_Semantic_Tangent:
            return VROGeometrySourceSemantic::Tangent;
        case viro::Node_Geometry_Source_Semantic_VertexCrease:
            return VROGeometrySourceSemantic::VertexCrease;
        case viro::Node_Geometry_Source_Semantic_EdgeCrease:
            return VROGeometrySourceSemantic::EdgeCrease;
        case viro::Node_Geometry_Source_Semantic_BoneWeights:
            return VROGeometrySourceSemantic::BoneWeights;
        case viro::Node_Geometry_Source_Semantic_BoneIndices:
            return VROGeometrySourceSemantic::BoneIndices;
        default:
            pabort();
    }
}

VROGeometryPrimitiveType convert(viro::Node_Geometry_Element_Primitive primitive) {
    switch (primitive) {
        case viro::Node_Geometry_Element_Primitive_Triangle:
            return VROGeometryPrimitiveType::Triangle;
        case viro::Node_Geometry_Element_Primitive_TriangleStrip:
            return VROGeometryPrimitiveType::TriangleStrip;
        case viro::Node_Geometry_Element_Primitive_Line:
            return VROGeometryPrimitiveType::Line;
        case viro::Node_Geometry_Element_Primitive_Point:
            return VROGeometryPrimitiveType::Point;
        default:
            pabort();
    }
}

VROLightingModel convert(viro::Node_Geometry_Material_LightingModel lightingModel) {
    switch (lightingModel) {
        case viro::Node_Geometry_Material_LightingModel_Constant:
            return VROLightingModel::Constant;
        case viro::Node_Geometry_Material_LightingModel_Lambert:
            return VROLightingModel::Lambert;
        case viro::Node_Geometry_Material_LightingModel_Blinn:
            return VROLightingModel::Blinn;
        case viro::Node_Geometry_Material_LightingModel_Phong:
            return VROLightingModel::Phong;
        case viro::Node_Geometry_Material_LightingModel_PhysicallyBased:
            return VROLightingModel::PhysicallyBased;
        default:
            pabort();
    }
}

VROWrapMode convert(viro::Node_Geometry_Material_Visual_WrapMode wrapMode) {
    switch (wrapMode) {
        case viro::Node_Geometry_Material_Visual_WrapMode_Clamp:
            return VROWrapMode::Clamp;
        case viro::Node_Geometry_Material_Visual_WrapMode_ClampToBorder:
            return VROWrapMode::ClampToBorder;
        case viro::Node_Geometry_Material_Visual_WrapMode_Mirror:
            return VROWrapMode::Mirror;
        case viro::Node_Geometry_Material_Visual_WrapMode_Repeat:
            return VROWrapMode::Repeat;
        default:
            pabort();
    }
}

VROFilterMode convert(viro::Node_Geometry_Material_Visual_FilterMode filterMode) {
    switch (filterMode) {
        case viro::Node_Geometry_Material_Visual_FilterMode_Linear:
            return VROFilterMode::Linear;
        case viro::Node_Geometry_Material_Visual_FilterMode_Nearest:
            return VROFilterMode::Nearest;
        case viro::Node_Geometry_Material_Visual_FilterMode_None:
            return VROFilterMode::None;
        default:
            pabort();
    }
}

void setTextureProperties(VROLightingModel lightingModel, const viro::Node::Geometry::Material::Visual &pb, std::shared_ptr<VROTexture> &texture) {
    // Temporary check: there is no way to set wrap modes and filters in FBX with PBR currently,
    // so force PBR materials to use the default (linear, linear, linear, clamp, clamp).
    if (lightingModel != VROLightingModel::PhysicallyBased) {
        texture->setMinificationFilter(convert(pb.minification_filter()));
        texture->setMagnificationFilter(convert(pb.magnification_filter()));
        texture->setMipFilter(convert(pb.mip_filter()));
        texture->setWrapS(convert(pb.wrap_mode_s()));
        texture->setWrapT(convert(pb.wrap_mode_t()));
        texture->setName(pb.texture().c_str());
    }
}

void VROFBXLoader::loadFBXFromResource(std::string resource, VROResourceType type,
                                       std::shared_ptr<VRONode> node,
                                       std::shared_ptr<VRODriver> driver,
                                       std::function<void(std::shared_ptr<VRONode>, bool)> onFinish) {
    
    VROModelIOUtil::retrieveResourceAsync(resource, type,
          [resource, type, node, driver, onFinish](std::string path, bool isTemp) {
              // onSuccess() (note: callbacks from retrieveResourceAsync occur on rendering thread)
              readFBXProtobufAsync(resource, type, node, path, isTemp, false, {}, driver, onFinish);
          },
          [node, onFinish]() {
              // onFailure()
              onFinish(node, false);
          });
}

void VROFBXLoader::loadFBXFromResources(std::string resource, VROResourceType type,
                                        std::shared_ptr<VRONode> node,
                                        std::map<std::string, std::string> resourceMap,
                                        std::shared_ptr<VRODriver> driver,
                                        std::function<void(std::shared_ptr<VRONode>, bool)> onFinish) {
    VROModelIOUtil::retrieveResourceAsync(resource, type,
          [resource, type, node, resourceMap, driver, onFinish](std::string path, bool isTemp) {
              // onSuccess (rendering thread)
              readFBXProtobufAsync(resource, type, node, path, isTemp, true, resourceMap, driver, onFinish);
          },
          [node, onFinish]() {
              onFinish(node, false);
          });
}

void VROFBXLoader::injectFBX(std::shared_ptr<VRONode> fbxNode,
                             std::shared_ptr<VRONode> node,
                             std::shared_ptr<VRODriver> driver,
                             std::function<void(std::shared_ptr<VRONode> node, bool success)> onFinish) {
    if (fbxNode) {
        // The top-level fbxNode is a dummy; all of the data is stored in the children, so we
        // simply transfer those children over to the destination node
        for (std::shared_ptr<VRONode> child : fbxNode->getChildNodes()) {
            node->addChildNode(child);
        }
        
        // Recompute the node's umbrellaBoundingBox and set the atomic rendering properties before
        // we notify the user that their FBX has finished loading
        node->recomputeUmbrellaBoundingBox();
        node->syncAppThreadProperties();
        node->setIgnoreEventHandling(node->getIgnoreEventHandling());

        // Hydrate the geometry and all textures prior to rendering, and ensure the callback is
        // invoked before rendering (this way position/rotation can be set in the callback without
        // causing any flicker).
        node->setHoldRendering(true);

        // Don't hold a strong reference to the Node: hydrateAsync stores its callback (and
        // thus all copied lambda variables, including the Node) in VROTexture, which can
        // expose us to strong reference cycles (Node <--> Texture). While the callback is
        // cleaned up after hydration completes, it's possible that hydrate will never
        // complete if the Node is quickly removed.
        std::weak_ptr<VRONode> node_w = node;
        VROModelIOUtil::hydrateAsync(node, [node_w, onFinish] {
            std::shared_ptr<VRONode> node_s = node_w.lock();
            if (node_s) {
                if (onFinish) {
                    onFinish(node_s, true);
                }
                node_s->setHoldRendering(false);
            }
        }, driver);
    }
    else {
        if (onFinish) {
            onFinish(node, false);
        }
    }
}

void VROFBXLoader::readFBXProtobufAsync(std::string resource, VROResourceType type, std::shared_ptr<VRONode> node,
                                        std::string path, bool isTemp, bool loadingTexturesFromResourceMap,
                                        std::map<std::string, std::string> resourceMap,
                                        std::shared_ptr<VRODriver> driver,
                                        std::function<void(std::shared_ptr<VRONode> node, bool success)> onFinish) {
    VROPlatformDispatchAsyncBackground([resource, type, node, path, resourceMap, driver, onFinish, isTemp, loadingTexturesFromResourceMap] {
        pinfo("Loading FBX from file %s", path.c_str());

        std::string data_pb_gzip = VROPlatformLoadFileAsString(path);
        if (!data_pb_gzip.empty()) {
            google::protobuf::io::ArrayInputStream input(data_pb_gzip.data(), (int) data_pb_gzip.size());
            google::protobuf::io::GzipInputStream gzipIn(&input);
            
            std::shared_ptr<viro::Node> node_pb = std::make_shared<viro::Node>();
            if (node_pb->ParseFromZeroCopyStream(&gzipIn)) {
                if (kDebugFBXLoading) {
                    pinfo("Read FBX protobuf");
                }

                /*
                 If the ancillary resources (e.g. textures) required by the model are provided in a
                 resource map, then generate the corresponding fileMap (this copies those resources
                 into local files).
                 */
                std::shared_ptr<std::map<std::string, std::string>> fileMap;
                if (loadingTexturesFromResourceMap) {
                    fileMap = VROModelIOUtil::createResourceMap(resourceMap, type);
                }

                VROPlatformDispatchAsyncRenderer(
                        [node, node_pb, resource, type, loadingTexturesFromResourceMap, fileMap, driver, onFinish] {
                            std::string base = resource.substr(0, resource.find_last_of('/'));

                            // Load the FBX from the protobuf on the rendering thread, accumulating additional
                            // tasks (e.g. async texture download) in the task queue
                            std::shared_ptr<VROTaskQueue> taskQueue = std::make_shared<VROTaskQueue>(
                                    "fbx", VROTaskExecutionOrder::Serial);
                            
                            // Add the task queue to the node so it doesn't get deleted until the model
                            // is loaded
                            node->addTaskQueue(taskQueue);


                            std::shared_ptr<std::map<std::string, std::shared_ptr<VROTexture>>> textureCache = std::make_shared<std::map<std::string, std::shared_ptr<VROTexture>>>();
                            std::shared_ptr<VRONode> fbxNode = loadFBX(*node_pb, node, base,
                                                                       loadingTexturesFromResourceMap
                                                                       ? VROResourceType::LocalFile
                                                                       : type,
                                                                       loadingTexturesFromResourceMap
                                                                       ? fileMap : nullptr,
                                                                       textureCache, taskQueue, driver);

                            // Run all the async tasks. When they're complete, inject the finished FBX into the
                            // node
                            std::weak_ptr<VRONode> node_w = node;
                            std::weak_ptr<VROTaskQueue> taskQueue_w = taskQueue;
                            taskQueue->processTasksAsync(
                                    [fbxNode, node_w, taskQueue_w, textureCache, node_pb, driver, onFinish] {
                                        std::shared_ptr<VRONode> node_s = node_w.lock();
                                        if (node_s) {
                                            injectFBX(fbxNode, node_s, driver, onFinish);
                                            
                                            std::shared_ptr<VROTaskQueue> taskQueue_s = taskQueue_w.lock();
                                            if (taskQueue_s) {
                                                node_s->removeTaskQueue(taskQueue_s);
                                            }
                                        }
                                    });
                        });
            } else {
                pinfo("Failed to parse FBX protobuf");
                onFinish(node, false);
            }
        }
        else {
            pinfo("Failed to load FBX protobuf data");
            onFinish(node, false);
        }
        if (isTemp) {
            VROPlatformDeleteFile(path);
        }
    });
}

std::shared_ptr<VRONode> VROFBXLoader::loadFBX(viro::Node &node_pb, std::shared_ptr<VRONode> finalRootNode,
                                               std::string base, VROResourceType type,
                                               std::shared_ptr<std::map<std::string, std::string>> resourceMap,
                                               std::shared_ptr<std::map<std::string, std::shared_ptr<VROTexture>>> textureCache,
                                               std::shared_ptr<VROTaskQueue> taskQueue,
                                               std::shared_ptr<VRODriver> driver) {
    
    // The root node contains the skeleton, if any
    std::shared_ptr<VROSkeleton> skeleton;
    if (node_pb.has_skeleton()) {
        skeleton = loadFBXSkeleton(node_pb.skeleton());
    }
    
    // The outer node of the protobuf has no mesh data, it contains
    // metadata (like the skeleton) and holds the root nodes of the
    // FBX mesh. We use our outer VRONode for the same purpose, to
    // contain the root nodes of the FBX file
    std::shared_ptr<VRONode> tempRootNode = std::make_shared<VRONode>();
    for (int i = 0; i < node_pb.subnode_size(); i++) {
        std::shared_ptr<VRONode> node = loadFBXNode(node_pb.subnode(i), skeleton, base, type,
                                                    resourceMap, textureCache, taskQueue, driver);
        tempRootNode->addChildNode(node);
    }
    trimEmptyNodes(tempRootNode);
    
    return tempRootNode;
}

std::shared_ptr<VRONode> VROFBXLoader::loadFBXNode(const viro::Node &node_pb,
                                                   std::shared_ptr<VROSkeleton> skeleton,
                                                   std::string base, VROResourceType type,
                                                   std::shared_ptr<std::map<std::string, std::string>> resourceMap,
                                                   std::shared_ptr<std::map<std::string, std::shared_ptr<VROTexture>>> textureCache,
                                                   std::shared_ptr<VROTaskQueue> taskQueue,
                                                   std::shared_ptr<VRODriver> driver) {
    
    if (kDebugFBXLoading) {
        pinfo("Loading node [%s]", node_pb.name().c_str());
    }
    
    std::shared_ptr<VRONode> node = std::make_shared<VRONode>();
    node->setName(node_pb.name());
    node->setPosition({ node_pb.position(0), node_pb.position(1), node_pb.position(2) });
    node->setScale({ node_pb.scale(0), node_pb.scale(1), node_pb.scale(2) });
    node->setRotation({ (float) degrees_to_radians(node_pb.rotation(0)),
                        (float) degrees_to_radians(node_pb.rotation(1)),
                        (float) degrees_to_radians(node_pb.rotation(2)) });
    node->setRenderingOrder(node_pb.rendering_order());
    node->setOpacity(node_pb.opacity());
    
    if (node_pb.has_geometry()) {
        const viro::Node_Geometry &geo_pb = node_pb.geometry();
        std::shared_ptr<VROGeometry> geo = loadFBXGeometry(geo_pb, base, type, resourceMap, textureCache, taskQueue, driver);
        geo->setName(node_pb.name());
        
        if (geo_pb.has_skin() && skeleton) {
            std::shared_ptr<VROSkinner> skinner = loadFBXSkinner(geo_pb.skin(), skeleton, driver);
            geo->setSkinner(skinner);
            skinner->setSkinnerNode(node);
            skeleton->setSkinnerRootNode(node);
            
            bool hasScaling = false;
            for (int i = 0; i < node_pb.skeletal_animation_size(); i++) {
                const viro::Node::SkeletalAnimation &animation_pb = node_pb.skeletal_animation(i);
                if (animation_pb.has_scaling()) {
                    hasScaling = true;
                }
                
                std::shared_ptr<VROSkeletalAnimation> animation = loadFBXSkeletalAnimation(animation_pb, skinner);
                if (animation->getName().empty()) {
                    animation->setName("fbx_skel_animation_" + VROStringUtil::toString(i));
                }
                
                node->addAnimation(animation->getName(), animation);
                if (kDebugFBXLoading) {
                    pinfo("   Added skeletal animation [%s]", animation->getName().c_str());
                }
                
                // Set the skinner to use the first frame's transform of the first animation; this is
                // so that we get a natural pose before any animation is run (otherwise the bone transforms
                // default to identity, which can give odd results).
                if (i == 0 && !animation->getFrames().empty()) {
                    const std::unique_ptr<VROSkeletalAnimationFrame> &frame = animation->getFrames().front();
                    for (int f = 0; f < frame->boneIndices.size(); f++) {
                        std::shared_ptr<VROBone> bone = skeleton->getBone(frame->boneIndices[f]);
                        bone->setTransform(frame->boneTransforms[f], frame->boneTransformsLegacy ? VROBoneTransformType::Legacy : VROBoneTransformType::Concatenated);
                    }
                }
            }
            
            if (hasScaling) {
                if (kDebugFBXLoading) {
                    pinfo("   At least 1 animation has scaling: using DQ+S modifier");
                }
            }
            
            for (const std::shared_ptr<VROMaterial> &material : geo->getMaterials()) {
                material->addShaderModifier(VROBoneUBO::createSkinningShaderModifier(hasScaling));
            }
        }
        
        node->setGeometry(geo);
    }
    
    for (int i = 0; i < node_pb.keyframe_animation_size(); i++) {
        const viro::Node::KeyframeAnimation &animation_pb = node_pb.keyframe_animation(i);
        std::shared_ptr<VROKeyframeAnimation> animation = loadFBXKeyframeAnimation(animation_pb);
        
        if (animation->getName().empty()) {
            animation->setName("fbx_kf_animation_" + VROStringUtil::toString(i));
        }
        
        node->addAnimation(animation->getName(), animation);
        if (kDebugFBXLoading) {
            pinfo("   Added keyframe animation [%s]", animation->getName().c_str());
        }
    }
    
    for (int i = 0; i < node_pb.subnode_size(); i++) {
        std::shared_ptr<VRONode> subnode = loadFBXNode(node_pb.subnode(i), skeleton, base, type,
                                                       resourceMap, textureCache, taskQueue, driver);
        node->addChildNode(subnode);
    }
    
    return node;
}

std::shared_ptr<VROGeometry> VROFBXLoader::loadFBXGeometry(const viro::Node_Geometry &geo_pb,
                                                           std::string base, VROResourceType type,
                                                           std::shared_ptr<std::map<std::string, std::string>> resourceMap,
                                                           std::shared_ptr<std::map<std::string, std::shared_ptr<VROTexture>>> textureCache,
                                                           std::shared_ptr<VROTaskQueue> taskQueue,
                                                           std::shared_ptr<VRODriver> driver) {
    std::shared_ptr<VROData> varData = std::make_shared<VROData>(geo_pb.data().c_str(), geo_pb.data().length());
    std::shared_ptr<VROVertexBuffer> vertexBuffer = driver->newVertexBuffer(varData);
    
    std::vector<std::shared_ptr<VROGeometrySource>> sources;
    for (int i = 0; i < geo_pb.source_size(); i++) {
        const viro::Node::Geometry::Source &source_pb = geo_pb.source(i);
        std::shared_ptr<VROGeometrySource> source = std::make_shared<VROGeometrySource>(vertexBuffer,
                                                                                        convert(source_pb.semantic()),
                                                                                        source_pb.vertex_count(),
                                                                                        source_pb.float_components(),
                                                                                        source_pb.components_per_vertex(),
                                                                                        source_pb.bytes_per_component(),
                                                                                        source_pb.data_offset(),
                                                                                        source_pb.data_stride());
        sources.push_back(source);
    }
    
    std::vector<std::shared_ptr<VROGeometryElement>> elements;
    for (int i = 0; i < geo_pb.element_size(); i++) {
        const viro::Node::Geometry::Element &element_pb = geo_pb.element(i);
        
        std::shared_ptr<VROData> data = std::make_shared<VROData>(element_pb.data().c_str(), element_pb.data().length());
        std::shared_ptr<VROGeometryElement> element = std::make_shared<VROGeometryElement>(data,
                                                                                           convert(element_pb.primitive()),
                                                                                           element_pb.primitive_count(),
                                                                                           element_pb.bytes_per_index());
        elements.push_back(element);
    }
    
    std::shared_ptr<VROGeometry> geo = std::make_shared<VROGeometry>(sources, elements);
    geo->setName(geo_pb.name());
    
    std::vector<std::shared_ptr<VROMaterial>> materials;
    for (int i = 0; i < geo_pb.material_size(); i++) {
        const viro::Node::Geometry::Material &material_pb = geo_pb.material(i);
        
        std::shared_ptr<VROMaterial> material = std::make_shared<VROMaterial>();
        material->setName(material_pb.name());
        material->setShininess(material_pb.shininess());
        material->setFresnelExponent(material_pb.fresnel_exponent());
        material->setTransparency(material_pb.transparency());
        material->setLightingModel(convert(material_pb.lighting_model()));
        material->setReadsFromDepthBuffer(true);
        material->setWritesToDepthBuffer(true);

        std::weak_ptr<VROMaterial> material_w = material;

        VROLightingModel lightingModel = material->getLightingModel();
        if (material_pb.has_diffuse()) {
            const viro::Node::Geometry::Material::Visual &diffuse_pb = material_pb.diffuse();
            VROMaterialVisual &diffuse = material->getDiffuse();
            
            if (diffuse_pb.color_size() >= 2) {
                diffuse.setColor({ diffuse_pb.color(0), diffuse_pb.color(1), diffuse_pb.color(2), 1.0 });
            }
            diffuse.setIntensity(diffuse_pb.intensity());
            
            std::weak_ptr<VROTaskQueue> taskQueue_w = taskQueue;
            
            if (!diffuse_pb.texture().empty()) {
                taskQueue->addTask([material_w, &diffuse_pb, lightingModel, base, type, resourceMap, textureCache, taskQueue_w] {
                    VROModelIOUtil::loadTextureAsync(diffuse_pb.texture(), base, type, true, resourceMap, textureCache,
                       [material_w, &diffuse_pb, lightingModel, taskQueue_w](std::shared_ptr<VROTexture> texture) {
                           std::shared_ptr<VROMaterial> material_s = material_w.lock();
                           if (material_s) {
                               if (texture) {
                                   material_s->getDiffuse().setTexture(texture);
                                   setTextureProperties(lightingModel, diffuse_pb, texture);
                               } else {
                                   pinfo("FBX failed to load diffuse texture [%s]", diffuse_pb.texture().c_str());
                               }
                           }
                           std::shared_ptr<VROTaskQueue> taskQueue = taskQueue_w.lock();
                           if (taskQueue) {
                               taskQueue->onTaskComplete();
                           }
                       });
                });
            }
        }
        if (material_pb.has_specular()) {
            const viro::Node::Geometry::Material::Visual &specular_pb = material_pb.specular();
            VROMaterialVisual &specular = material->getSpecular();
            specular.setIntensity(specular_pb.intensity());
            
            std::weak_ptr<VROTaskQueue> taskQueue_w = taskQueue;

            if (!specular_pb.texture().empty()) {
                taskQueue->addTask([material_w, &specular_pb, lightingModel, base, type, resourceMap, textureCache, taskQueue_w] {
                    VROModelIOUtil::loadTextureAsync(specular_pb.texture(), base, type, false, resourceMap, textureCache,
                        [material_w, &specular_pb, lightingModel, taskQueue_w](std::shared_ptr<VROTexture> texture) {
                            std::shared_ptr<VROMaterial> material_s = material_w.lock();
                            if (material_s) {
                                if (texture) {
                                    material_s->getSpecular().setTexture(texture);
                                    setTextureProperties(lightingModel, specular_pb, texture);
                                } else {
                                    pinfo("FBX failed to load specular texture [%s]", specular_pb.texture().c_str());
                                }
                            }
                            std::shared_ptr<VROTaskQueue> taskQueue = taskQueue_w.lock();
                            if (taskQueue) {
                                taskQueue->onTaskComplete();
                            }
                        });
                });
            }
        }
        if (material_pb.has_normal()) {
            const viro::Node::Geometry::Material::Visual &normal_pb = material_pb.normal();
            VROMaterialVisual &normal = material->getNormal();
            normal.setIntensity(normal_pb.intensity());
            
            std::weak_ptr<VROTaskQueue> taskQueue_w = taskQueue;

            if (!normal_pb.texture().empty()) {
                taskQueue->addTask([material_w, &normal_pb, lightingModel, base, type, resourceMap, textureCache, taskQueue_w] {
                    VROModelIOUtil::loadTextureAsync(normal_pb.texture(), base, type, false, resourceMap, textureCache,
                         [material_w, &normal_pb, lightingModel, taskQueue_w](std::shared_ptr<VROTexture> texture) {
                             std::shared_ptr<VROMaterial> material_s = material_w.lock();
                             if (material_s) {
                                 if (texture) {
                                     material_s->getNormal().setTexture(texture);
                                     setTextureProperties(lightingModel, normal_pb, texture);
                                 } else {
                                     pinfo("FBX failed to load normal texture [%s]", normal_pb.texture().c_str());
                                 }
                             }
                             std::shared_ptr<VROTaskQueue> taskQueue = taskQueue_w.lock();
                             if (taskQueue) {
                                 taskQueue->onTaskComplete();
                             }
                         });
                });
            }
        }
        if (material_pb.has_roughness()) {
            const viro::Node::Geometry::Material::Visual &roughness_pb = material_pb.roughness();
            VROMaterialVisual &roughness = material->getRoughness();
            roughness.setIntensity(roughness_pb.intensity());

            std::weak_ptr<VROTaskQueue> taskQueue_w = taskQueue;

            if (!roughness_pb.texture().empty()) {
                taskQueue->addTask([material_w, &roughness_pb, lightingModel, base, type, resourceMap, textureCache, taskQueue_w] {
                    VROModelIOUtil::loadTextureAsync(roughness_pb.texture(), base, type, false, resourceMap, textureCache,
                        [material_w, &roughness_pb, lightingModel, taskQueue_w](std::shared_ptr<VROTexture> texture) {
                            std::shared_ptr<VROMaterial> material_s = material_w.lock();
                            if (material_s) {
                                if (texture) {
                                    material_s->getRoughness().setTexture(texture);
                                    setTextureProperties(lightingModel, roughness_pb, texture);
                                } else {
                                    pinfo("FBX failed to load roughness texture [%s]", roughness_pb.texture().c_str());
                                }
                            }
                            std::shared_ptr<VROTaskQueue> taskQueue = taskQueue_w.lock();
                            if (taskQueue) {
                                taskQueue->onTaskComplete();
                            }
                        });
                });
            }
            else {
                roughness.setColor({ roughness_pb.color(0), 0, 0, 0 });
            }
        }
        if (material_pb.has_metalness()) {
            const viro::Node::Geometry::Material::Visual &metalness_pb = material_pb.metalness();
            VROMaterialVisual &metalness = material->getMetalness();
            metalness.setIntensity(metalness_pb.intensity());

            std::weak_ptr<VROMaterial> material_w = material;
            std::weak_ptr<VROTaskQueue> taskQueue_w = taskQueue;
            
            if (!metalness_pb.texture().empty()) {
                taskQueue->addTask([material_w, &metalness_pb, lightingModel, base, type, resourceMap, textureCache, taskQueue_w] {
                    VROModelIOUtil::loadTextureAsync(metalness_pb.texture(), base, type, false, resourceMap, textureCache,
                         [material_w, &metalness_pb, lightingModel, taskQueue_w](std::shared_ptr<VROTexture> texture) {
                             std::shared_ptr<VROMaterial> material_s = material_w.lock();
                             if (material_s) {
                                 if (texture) {
                                     material_s->getMetalness().setTexture(texture);
                                     setTextureProperties(lightingModel, metalness_pb, texture);
                                 } else {
                                     pinfo("FBX failed to load metalness texture [%s]", metalness_pb.texture().c_str());
                                 }
                             }
                             std::shared_ptr<VROTaskQueue> taskQueue = taskQueue_w.lock();
                             if (taskQueue) {
                                 taskQueue->onTaskComplete();
                             }
                         });
                });
            }
            else {
                metalness.setColor({ metalness_pb.color(0), 0, 0, 0 });
            }
        }
        if (material_pb.has_ao()) {
            const viro::Node::Geometry::Material::Visual &ao_pb = material_pb.ao();
            VROMaterialVisual &ao = material->getAmbientOcclusion();
            ao.setIntensity(ao_pb.intensity());
            
            std::weak_ptr<VROTaskQueue> taskQueue_w = taskQueue;
            
            if (!ao_pb.texture().empty()) {
                taskQueue->addTask([material_w, &ao_pb, lightingModel, base, type, resourceMap, textureCache, taskQueue_w] {
                    VROModelIOUtil::loadTextureAsync(ao_pb.texture(), base, type, true, resourceMap, textureCache,
                         [material_w, &ao_pb, lightingModel, taskQueue_w](std::shared_ptr<VROTexture> texture) {;
                             std::shared_ptr<VROMaterial> material_s = material_w.lock();
                             if (material_s) {
                                 if (texture) {
                                     material_s->getAmbientOcclusion().setTexture(texture);
                                     setTextureProperties(lightingModel, ao_pb, texture);
                                 } else {
                                     pinfo("FBX failed to load AO texture [%s]", ao_pb.texture().c_str());
                                 }
                             }
                             std::shared_ptr<VROTaskQueue> taskQueue = taskQueue_w.lock();
                             if (taskQueue) {
                                 taskQueue->onTaskComplete();
                             }
                         });
                });
            }
        }
        
        materials.push_back(material);
    }
    geo->setMaterials(materials);
    
    VROBoundingBox bounds = geo->getBoundingBox();
    pinfo("   Bounds x(%f %f)", bounds.getMinX(), bounds.getMaxX());
    pinfo("          y(%f %f)", bounds.getMinY(), bounds.getMaxY());
    pinfo("          z(%f %f)", bounds.getMinZ(), bounds.getMaxZ());
    
    return geo;
}

std::shared_ptr<VROSkeleton> VROFBXLoader::loadFBXSkeleton(const viro::Node_Skeleton &skeleton_pb) {
    std::vector<std::shared_ptr<VROBone>> bones;
    for (int i = 0; i < skeleton_pb.bone_size(); i++) {
        int parentIndex = skeleton_pb.bone(i).parent_index();
        std::string name = skeleton_pb.bone(i).name();
        
        VROMatrix4f boneLocalTransform;
        if (skeleton_pb.bone(i).has_local_transform() > 0) {
            for (int m = 0; m < 16; m++) {
                boneLocalTransform[m] = skeleton_pb.bone(i).local_transform().value(m);
            }
        }

        VROMatrix4f boneSpaceBindTransform;
        if (skeleton_pb.bone(i).has_bind_transform() > 0) {
            for (int m = 0; m < 16; m++) {
                boneSpaceBindTransform[m] = skeleton_pb.bone(i).bind_transform().value(m);
            }
        }

        // Create attachment transforms associated with this bone, if any.
        std::map<std::string, VROMatrix4f> attachmentTransforms;
        int attachmentTransformsSize = skeleton_pb.bone(i).attachment_transforms_size();
        for (int b = 0; b < attachmentTransformsSize; b ++) {
            std::string key = skeleton_pb.bone(i).attachment_transforms(b).key();
            VROMatrix4f boneAttachmentTransform;
            for (int m = 0; m < 16; m++) {
                boneAttachmentTransform[m] = skeleton_pb.bone(i).attachment_transforms(b).value().value(m);
            }
            attachmentTransforms[key] = boneAttachmentTransform;
        }

        std::shared_ptr<VROBone> bone = std::make_shared<VROBone>(i,
                                                                  parentIndex,
                                                                  name,
                                                                  boneLocalTransform,
                                                                  boneSpaceBindTransform);
        bone->setAttachmentTransforms(attachmentTransforms);
        bones.push_back(bone);
    }
    
    return std::make_shared<VROSkeleton>(bones);
}

std::shared_ptr<VROSkinner> VROFBXLoader::loadFBXSkinner(const viro::Node_Geometry_Skin &skin_pb,
                                                         std::shared_ptr<VROSkeleton> skeleton,
                                                         std::shared_ptr<VRODriver> driver) {
    
    float geometryBindMtx[16];
    for (int j = 0; j < 16; j++) {
        geometryBindMtx[j] = skin_pb.geometry_bind_transform().value(j);
    }
    VROMatrix4f geometryBindTransform(geometryBindMtx);
    
    std::vector<VROMatrix4f> bindTransforms;
    for (int i = 0; i < skin_pb.bind_transform_size(); i++) {
        
        if (skin_pb.bind_transform(i).value_size() != 16) {
            // Push identity if we don't have a bind transform for this bone
            bindTransforms.push_back(VROMatrix4f::identity());
        }
        else {
            float mtx[16];
            for (int j = 0; j < 16; j++) {
                mtx[j] = skin_pb.bind_transform(i).value(j);
            }
            bindTransforms.push_back({ mtx });
        }
    }
    
    const viro::Node::Geometry::Source &bone_indices_pb = skin_pb.bone_indices();
    std::shared_ptr<VROData> boneIndicesData = std::make_shared<VROData>(bone_indices_pb.data().c_str(), bone_indices_pb.data().length());
    std::shared_ptr<VROGeometrySource> boneIndices = std::make_shared<VROGeometrySource>(driver->newVertexBuffer(boneIndicesData),
                                                                                         convert(bone_indices_pb.semantic()),
                                                                                         bone_indices_pb.vertex_count(),
                                                                                         bone_indices_pb.float_components(),
                                                                                         bone_indices_pb.components_per_vertex(),
                                                                                         bone_indices_pb.bytes_per_component(),
                                                                                         bone_indices_pb.data_offset(),
                                                                                         bone_indices_pb.data_stride());
    
    const viro::Node::Geometry::Source &bone_weights_pb = skin_pb.bone_weights();
    std::shared_ptr<VROData> boneWeightsData = std::make_shared<VROData>(bone_weights_pb.data().c_str(), bone_weights_pb.data().length());
    std::shared_ptr<VROGeometrySource> boneWeights = std::make_shared<VROGeometrySource>(driver->newVertexBuffer(boneWeightsData),
                                                                                         convert(bone_weights_pb.semantic()),
                                                                                         bone_weights_pb.vertex_count(),
                                                                                         bone_weights_pb.float_components(),
                                                                                         bone_weights_pb.components_per_vertex(),
                                                                                         bone_weights_pb.bytes_per_component(),
                                                                                         bone_weights_pb.data_offset(),
                                                                                         bone_weights_pb.data_stride());

    return std::shared_ptr<VROSkinner>(new VROSkinner(skeleton, geometryBindTransform, bindTransforms, boneIndices, boneWeights));
}

std::shared_ptr<VROSkeletalAnimation> VROFBXLoader::loadFBXSkeletalAnimation(const viro::Node_SkeletalAnimation &animation_pb,
                                                                             std::shared_ptr<VROSkinner> skinner) {
    
    std::vector<std::unique_ptr<VROSkeletalAnimationFrame>> frames;
    for (int f = 0; f < animation_pb.frame_size(); f++) {
        const viro::Node::SkeletalAnimation::Frame &frame_pb = animation_pb.frame(f);
        
        std::unique_ptr<VROSkeletalAnimationFrame> frame = std::unique_ptr<VROSkeletalAnimationFrame>(new VROSkeletalAnimationFrame());
        frame->boneTransformsLegacy = true;
        frame->time = frame_pb.time();
        
        passert (frame_pb.bone_index_size() == frame_pb.transform_size());
        for (int b = 0; b < frame_pb.bone_index_size(); b++) {
            frame->boneIndices.push_back(frame_pb.bone_index(b));
            
            float mtx[16];
            for (int i = 0; i < 16; i++) {
                mtx[i] = frame_pb.transform(b).value(i);
            }
            frame->boneTransforms.push_back({ mtx });
            
            if (frame_pb.local_transform_size() > 0) {
                for (int i = 0; i < 16; i++) {
                    mtx[i] = frame_pb.local_transform(b).value(i);
                }
                frame->localBoneTransforms.push_back( { mtx });
                
                // If we have local bone transforms, that also indicates our concatenated
                // bone transforms are not legacy
                frame->boneTransformsLegacy = false;
            }
        }
        
        frames.push_back(std::move(frame));
    }
    
    float duration = animation_pb.duration();
    
    std::shared_ptr<VROSkeletalAnimation> animation = std::make_shared<VROSkeletalAnimation>(skinner, frames, duration / 1000.0);
    animation->setName(animation_pb.name());
    
    return animation;
}

std::shared_ptr<VROKeyframeAnimation> VROFBXLoader::loadFBXKeyframeAnimation(const viro::Node_KeyframeAnimation &animation_pb) {
    std::vector<std::unique_ptr<VROKeyframeAnimationFrame>> frames;
    bool hasTranslation = false;
    bool hasRotation = false;
    bool hasScale = false;
    
    for (int f = 0; f < animation_pb.frame_size(); f++) {
        const viro::Node::KeyframeAnimation::Frame &frame_pb = animation_pb.frame(f);
        
        std::unique_ptr<VROKeyframeAnimationFrame> frame = std::unique_ptr<VROKeyframeAnimationFrame>(new VROKeyframeAnimationFrame());
        frame->time = frame_pb.time();
        
        if (frame_pb.translation_size() >= 2) {
            hasTranslation = true;
            frame->translation = { frame_pb.translation(0), frame_pb.translation(1), frame_pb.translation(2) };
        }
        if (frame_pb.scale_size() >= 2) {
            hasScale = true;
            frame->scale = { frame_pb.scale(0), frame_pb.scale(1), frame_pb.scale(2) };
        }
        if (frame_pb.rotation_size() >= 3) {
            hasRotation = true;
            frame->rotation = { frame_pb.rotation(0), frame_pb.rotation(1), frame_pb.rotation(2), frame_pb.rotation(3) };
        }
        frames.push_back(std::move(frame));
    }
    
    float duration = animation_pb.duration();
    
    std::shared_ptr<VROKeyframeAnimation> animation = std::make_shared<VROKeyframeAnimation>(frames, duration / 1000.0, hasTranslation, hasRotation, hasScale, false);
    animation->setName(animation_pb.name());
    
    return animation;
}

void VROFBXLoader::trimEmptyNodes(std::shared_ptr<VRONode> node) {
    std::vector<std::shared_ptr<VRONode>> toRemove;
    for (std::shared_ptr<VRONode> &child : node->getChildNodes()) {
        if (!nodeHasGeometryRecursive(child)) {
            toRemove.push_back(child);
        }
    }
    
    for (std::shared_ptr<VRONode> &r : toRemove) {
        r->removeFromParentNode();
    }
    
    for (std::shared_ptr<VRONode> &child : node->getChildNodes()) {
        trimEmptyNodes(child);
    }
}

bool VROFBXLoader::nodeHasGeometryRecursive(std::shared_ptr<VRONode> node) {
    if (node->getGeometry() != nullptr) {
        return true;
    }
    else {
        bool hasGeometryInChildren = false;
        for (std::shared_ptr<VRONode> &child : node->getChildNodes()) {
            if (nodeHasGeometryRecursive(child)) {
                hasGeometryInChildren = true;
            }
        }
        
        return hasGeometryInChildren;
    }
}
