//
//  VROSkybox.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 1/11/16.
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

#include "VROSkybox.h"
#include "VROData.h"
#include "VROMaterial.h"
#include "VROGeometrySource.h"
#include "VROGeometryElement.h"
#include "VROShapeUtils.h"

static const int kNumSkyboxVertices = 24;
static const int kNumSkyboxIndices = 36;
static const int kSkyboxStride = sizeof(VROShapeVertexLayout);

static const VROShapeVertexLayout vertices[] = {
    // +Y
    { -0.5,  0.5,  0.5,  1.0, 0.0,  0.0, -1.0, 0.0,  0.0, 0.0, 0.0, 1.0 },
    {  0.5,  0.5,  0.5,  1.0, 0.0,  0.0, -1.0, 0.0,  0.0, 0.0, 0.0, 1.0 },
    {  0.5,  0.5, -0.5,  1.0, 0.0,  0.0, -1.0, 0.0,  0.0, 0.0, 0.0, 1.0 },
    { -0.5,  0.5, -0.5,  1.0, 0.0,  0.0, -1.0, 0.0,  0.0, 0.0, 0.0, 1.0 },

    // -Y
    { -0.5, -0.5, -0.5,  1.0, 0.0,  0.0,  1.0, 0.0,  0.0, 0.0, 0.0, 1.0 },
    {  0.5, -0.5, -0.5,  1.0, 0.0,  0.0,  1.0, 0.0,  0.0, 0.0, 0.0, 1.0 },
    {  0.5, -0.5,  0.5,  1.0, 0.0,  0.0,  1.0, 0.0,  0.0, 0.0, 0.0, 1.0 },
    { -0.5, -0.5,  0.5,  1.0, 0.0,  0.0,  1.0, 0.0,  0.0, 0.0, 0.0, 1.0 },
    
    // +Z
    { -0.5, -0.5,  0.5,  1.0, 0.0,  0.0, 0.0, -1.0,  0.0, 0.0, 0.0, 1.0 },
    {  0.5, -0.5,  0.5,  1.0, 0.0,  0.0, 0.0, -1.0,  0.0, 0.0, 0.0, 1.0 },
    {  0.5,  0.5,  0.5,  1.0, 0.0,  0.0, 0.0, -1.0,  0.0, 0.0, 0.0, 1.0 },
    { -0.5,  0.5,  0.5,  1.0, 0.0,  0.0, 0.0, -1.0,  0.0, 0.0, 0.0, 1.0 },
    
    // -Z
    {  0.5, -0.5, -0.5,  1.0, 0.0,  0.0, 0.0,  1.0,  0.0, 0.0, 0.0, 1.0 },
    { -0.5, -0.5, -0.5,  1.0, 0.0,  0.0, 0.0,  1.0,  0.0, 0.0, 0.0, 1.0 },
    { -0.5,  0.5, -0.5,  1.0, 0.0,  0.0, 0.0,  1.0,  0.0, 0.0, 0.0, 1.0 },
    {  0.5,  0.5, -0.5,  1.0, 0.0,  0.0, 0.0,  1.0,  0.0, 0.0, 0.0, 1.0 },

    // -X
    { -0.5, -0.5, -0.5,  1.0, 0.0,  1.0, 0.0,  0.0,  0.0, 0.0, 0.0, 1.0 },
    { -0.5, -0.5,  0.5,  1.0, 0.0,  1.0, 0.0,  0.0,  0.0, 0.0, 0.0, 1.0 },
    { -0.5,  0.5,  0.5,  1.0, 0.0,  1.0, 0.0,  0.0,  0.0, 0.0, 0.0, 1.0 },
    { -0.5,  0.5, -0.5,  1.0, 0.0,  1.0, 0.0,  0.0,  0.0, 0.0, 0.0, 1.0 },
    
    // +X
    {  0.5, -0.5,  0.5,  1.0, 0.0,  -1.0, 0.0, 0.0,  0.0, 0.0, 0.0, 1.0 },
    {  0.5, -0.5, -0.5,  1.0, 0.0,  -1.0, 0.0, 0.0,  0.0, 0.0, 0.0, 1.0 },
    {  0.5,  0.5, -0.5,  1.0, 0.0,  -1.0, 0.0, 0.0,  0.0, 0.0, 0.0, 1.0 },
    {  0.5,  0.5,  0.5,  1.0, 0.0,  -1.0, 0.0, 0.0,  0.0, 0.0, 0.0, 1.0 }
};

static uint32_t indices[] = {
    0,  3,  2,  2,  1,  0,
    4,  7,  6,  6,  5,  4,
    8, 11, 10, 10,  9,  8,
    12, 15, 14, 14, 13, 12,
    16, 19, 18, 18, 17, 16,
    20, 23, 22, 22, 21, 20,
};

std::shared_ptr<VROSkybox> VROSkybox::createSkybox(std::shared_ptr<VROTexture> textureCube) {
    std::shared_ptr<VROSkybox> skybox = buildSkyboxGeometry();
    
    std::shared_ptr<VROMaterial> material = std::make_shared<VROMaterial>();
    material->setWritesToDepthBuffer(false);
    material->getDiffuse().setTexture(textureCube);
    material->setLightingModel(VROLightingModel::Constant);
    material->setNeedsToneMapping(false);
    
    skybox->setMaterials({ material });
    skybox->setCameraEnclosure(true);
    return skybox;
}

std::shared_ptr<VROSkybox> VROSkybox::createSkybox(VROVector4f color) {
    std::shared_ptr<VROSkybox> skybox = buildSkyboxGeometry();
    
    std::shared_ptr<VROMaterial> material = std::make_shared<VROMaterial>();
    material->setWritesToDepthBuffer(false);
    material->getDiffuse().setColor(color);
    material->setLightingModel(VROLightingModel::Constant);
    material->setNeedsToneMapping(false);
    
    skybox->setMaterials({ material });
    skybox->setCameraEnclosure(true);
    return skybox;
}

VROSkybox::~VROSkybox() {
    
}

std::shared_ptr<VROSkybox> VROSkybox::buildSkyboxGeometry() {
    std::shared_ptr<VROData> vertexData = std::make_shared<VROData>((void *) vertices, kSkyboxStride * kNumSkyboxVertices);
    std::vector<std::shared_ptr<VROGeometrySource>> sources = VROShapeUtilBuildGeometrySources(vertexData, kNumSkyboxVertices);
    
    std::shared_ptr<VROData> indexData = std::make_shared<VROData>((void *) indices, sizeof(int32_t) * kNumSkyboxIndices);
    std::shared_ptr<VROGeometryElement> element = std::make_shared<VROGeometryElement>(indexData,
                                                                                       VROGeometryPrimitiveType::Triangle,
                                                                                       kNumSkyboxIndices / 3,
                                                                                       sizeof(int32_t));
    std::vector<std::shared_ptr<VROGeometryElement>> elements = { element };
    return std::shared_ptr<VROSkybox>(new VROSkybox(sources, elements));
}
