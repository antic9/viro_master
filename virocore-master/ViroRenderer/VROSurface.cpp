//
//  VROSurface.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 12/3/15.
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

#include "VROSurface.h"
#include "VROData.h"
#include "VROGeometrySource.h"
#include "VROGeometryElement.h"
#include "VROMaterial.h"
#include "VROAnimationFloat.h"
#include "VROLog.h"
#include "stdlib.h"

std::shared_ptr<VROSurface> VROSurface::createSurface(float width, float height,
                                                      float u0, float v0, float u1, float v1) {
    return createSurface(0, 0, width, height, u0, v0, u1, v1);
}

std::shared_ptr<VROSurface> VROSurface::createSurface(float x, float y, float width, float height,
                                                      float u0, float v0, float u1, float v1) {
    std::shared_ptr<VROSurface> surface = std::shared_ptr<VROSurface>(new VROSurface(x, y, width, height, u0, v0, u1, v1));
    std::shared_ptr<VROMaterial> material = std::make_shared<VROMaterial>();
    surface->setMaterials({ material });
    
    return surface;
}

VROSurface::VROSurface(float x, float y, float width, float height,
                       float u0, float v0, float u1, float v1) :
    _x(x),
    _y(y),
    _width(width),
    _height(height),
    _u0(u0),
    _v0(v0),
    _u1(u1),
    _v1(v1) {
    
    updateSurface();
}

VROSurface::~VROSurface() {
    
}

void VROSurface::updateSurface() {
    std::vector<std::shared_ptr<VROGeometrySource>> sources;
    std::vector<std::shared_ptr<VROGeometryElement>> elements;
    
    VROVector3f BL = { _u0, _v1, 0 };
    BL = _texcoordTransform.multiply(BL);
    
    VROVector3f BR = { _u1, _v1, 0 };
    BR = _texcoordTransform.multiply(BR);
    
    VROVector3f TR = { _u1, _v0, 0 };
    TR = _texcoordTransform.multiply(TR);

    VROVector3f TL = { _u0, _v0, 0 };
    TL = _texcoordTransform.multiply(TL);
    buildGeometry(_x, _y, _width, _height, BL, BR, TL, TR, sources, elements);
    
    setSources(sources);
    setElements(elements);
    updateBoundingBox();
}

void VROSurface::setWidth(float width) {
    animate(std::make_shared<VROAnimationFloat>([](VROAnimatable *const animatable, float v) {
        ((VROSurface *)animatable)->_width = v;
        ((VROSurface *)animatable)->updateSurface();
    }, _width, width));
}

void VROSurface::setHeight(float height) {
    animate(std::make_shared<VROAnimationFloat>([](VROAnimatable *const animatable, float v) {
        ((VROSurface *)animatable)->_height = v;
        ((VROSurface *)animatable)->updateSurface();
    }, _height, height));
}

void VROSurface::setX(float x) {
    animate(std::make_shared<VROAnimationFloat>([](VROAnimatable *const animatable, float v) {
        ((VROSurface *)animatable)->_x = v;
        ((VROSurface *)animatable)->updateSurface();
    }, _x, x));
}

void VROSurface::setY(float y) {
    animate(std::make_shared<VROAnimationFloat>([](VROAnimatable *const animatable, float v) {
        ((VROSurface *)animatable)->_y = v;
        ((VROSurface *)animatable)->updateSurface();
    }, _y, y));
}

void VROSurface::setU0(float u0) {
    animate(std::make_shared<VROAnimationFloat>([](VROAnimatable *const animatable, float v) {
        ((VROSurface *)animatable)->_u0 = v;
        ((VROSurface *)animatable)->updateSurface();
    }, _u0, u0));
}

void VROSurface::setU1(float u1) {
    animate(std::make_shared<VROAnimationFloat>([](VROAnimatable *const animatable, float v) {
        ((VROSurface *)animatable)->_u1 = v;
        ((VROSurface *)animatable)->updateSurface();
    }, _u1, u1));
}

void VROSurface::setV0(float v0) {
    animate(std::make_shared<VROAnimationFloat>([](VROAnimatable *const animatable, float v) {
        ((VROSurface *)animatable)->_v0 = v;
        ((VROSurface *)animatable)->updateSurface();
    }, _v0, v0));
}

void VROSurface::setV1(float v1) {
    animate(std::make_shared<VROAnimationFloat>([](VROAnimatable *const animatable, float v) {
        ((VROSurface *)animatable)->_v1 = v;
        ((VROSurface *)animatable)->updateSurface();
    }, _v1, v1));
}

void VROSurface::setTextureCoordinates(VROVector3f BL, VROVector3f BR, VROVector3f TL, VROVector3f TR) {
    std::vector<std::shared_ptr<VROGeometrySource>> sources;
    std::vector<std::shared_ptr<VROGeometryElement>> elements;
    buildGeometry(_x, _y, _width, _height, BL, BR, TL, TR, sources, elements);
    
    setSources(sources);
    setElements(elements);
    updateBoundingBox();
}

void VROSurface::setTexcoordTransform(VROMatrix4f transform) {
    _texcoordTransform = transform;
    updateSurface();
}

void VROSurface::buildGeometry(float x, float y, float width, float height,
                               VROVector3f texBL, VROVector3f texBR, VROVector3f texTL, VROVector3f texTR,
                               std::vector<std::shared_ptr<VROGeometrySource>> &sources,
                               std::vector<std::shared_ptr<VROGeometryElement>> &elements) {
    const int numVertices = 4;
    const int numIndices = 6;
    
    int varSizeBytes = sizeof(VROShapeVertexLayout) * numVertices;
    VROShapeVertexLayout var[varSizeBytes];
    
    VROSurface::buildSurface(var, x - width / 2.0, y - height / 2.0, x + width / 2.0, y + height / 2.0,
                             texBL, texBR, texTL, texTR);
    int indices[numIndices] = { 0, 1, 3, 2, 3, 1 };
    
    VROShapeUtilComputeTangents(var, numVertices, indices, numIndices);
    
    std::shared_ptr<VROData> vertexData = std::make_shared<VROData>((void *) var, varSizeBytes);
    std::vector<std::shared_ptr<VROGeometrySource>> genSources = VROShapeUtilBuildGeometrySources(vertexData, numVertices);
    for (std::shared_ptr<VROGeometrySource> source : genSources) {
        sources.push_back(source);
    }
    
    std::shared_ptr<VROData> indexData = std::make_shared<VROData>((void *) indices, sizeof(int) * 6);
    std::shared_ptr<VROGeometryElement> element = std::make_shared<VROGeometryElement>(indexData,
                                                                                       VROGeometryPrimitiveType::Triangle,
                                                                                       2,
                                                                                       sizeof(int));
    elements.push_back(element);
}

void VROSurface::buildSurface(VROShapeVertexLayout *vertexLayout,
                              float left, float bottom, float right, float top,
                              VROVector3f texBL, VROVector3f texBR, VROVector3f texTL, VROVector3f texTR) {
    float z = 0;
    
    vertexLayout[0].x = left;
    vertexLayout[0].y = bottom;
    vertexLayout[0].z = z;
    vertexLayout[0].u = texBL.x;
    vertexLayout[0].v = texBL.y;
    vertexLayout[0].nx = 0;
    vertexLayout[0].ny = 0;
    vertexLayout[0].nz = 1;
    
    vertexLayout[1].x = right;
    vertexLayout[1].y = bottom;
    vertexLayout[1].z = z;
    vertexLayout[1].u = texBR.x;
    vertexLayout[1].v = texBR.y;
    vertexLayout[1].nx = 0;
    vertexLayout[1].ny = 0;
    vertexLayout[1].nz = 1;
    
    vertexLayout[2].x = right;
    vertexLayout[2].y = top;
    vertexLayout[2].z = z;
    vertexLayout[2].u = texTR.x;
    vertexLayout[2].v = texTR.y;
    vertexLayout[2].nx = 0;
    vertexLayout[2].ny = 0;
    vertexLayout[2].nz = 1;
    
    vertexLayout[3].x = left;
    vertexLayout[3].y = top;
    vertexLayout[3].z = z;
    vertexLayout[3].u = texTL.x;
    vertexLayout[3].v = texTL.y;
    vertexLayout[3].nx = 0;
    vertexLayout[3].ny = 0;
    vertexLayout[3].nz = 1;
}

