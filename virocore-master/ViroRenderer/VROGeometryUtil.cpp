//
//  VROGeometryUtil.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 3/2/16.
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

#include "VROGeometryUtil.h"
#include "VROData.h"
#include "VROGeometrySource.h"
#include "VROGeometryElement.h"
#include "VROMath.h"
#include "VROLog.h"
#include "VRONode.h"
#include "VROGeometry.h"
#include <set>

std::vector<std::shared_ptr<VRONode>> VROGeometryUtilSplitNodeByGeometryElements(std::shared_ptr<VRONode> node) {
    std::vector<std::shared_ptr<VRONode>> nodes;
    std::shared_ptr<VROGeometry> geometry = node->getGeometry();
    
    for (int i = 0; i < geometry->getGeometryElements().size(); i++) {
        std::shared_ptr<VROGeometryElement> element = geometry->getGeometryElements()[i];
        std::vector<std::shared_ptr<VROGeometryElement>> elements = { element };
        
        std::shared_ptr<VROGeometrySource> vertexSource = geometry->getGeometrySourcesForSemantic(VROGeometrySourceSemantic::Vertex).front();
        
        VROVector3f center;
        std::shared_ptr<VROData> data = VROGeometryUtilExtractAndCenter(element, vertexSource, &center);
        
        std::vector<std::shared_ptr<VROGeometrySource>> sources;
        for (std::shared_ptr<VROGeometrySource> source : geometry->getGeometrySources()) {
            sources.push_back(std::make_shared<VROGeometrySource>(data, source));
        }
        
        std::shared_ptr<VROMaterial> material = geometry->getMaterials()[i];
        std::shared_ptr<VROGeometry> geometry = std::make_shared<VROGeometry>(sources, elements);
        geometry->setMaterials({ material });
        
        std::shared_ptr<VRONode> splitNode = std::make_shared<VRONode>();
        splitNode->setGeometry(geometry);
        splitNode->setPosition(center);
        
        nodes.push_back(splitNode);
    }
    
    return nodes;
}

std::shared_ptr<VROData> VROGeometryUtilExtractAndCenter(std::shared_ptr<VROGeometryElement> element,
                                                         std::shared_ptr<VROGeometrySource> geometrySource,
                                                         VROVector3f *outCenter) {
    std::vector<VROVector3f> allVertices;
    geometrySource->processVertices([&allVertices](int index, VROVector4f vertex) {
        allVertices.push_back(VROVector3f(vertex.x, vertex.y, vertex.z));
    });
    
    /*
     Find the center. Note that vertices may be referenced multiple times by
     an element, so we get the unique vertex indices first.
     */
    std::set<int> elementVertexIndices;
    element->processIndices([&allVertices, &elementVertexIndices](int index, int indexRead) {
        elementVertexIndices.insert(indexRead);
    });
    
    std::vector<VROVector3f> elementVertices;
    for (int elementVertexIndex : elementVertexIndices) {
        elementVertices.push_back(allVertices[elementVertexIndex]);
    }
    
    *outCenter = VROMathGetCenter(elementVertices);

    /*
     Subtract the center from each vertex in the element.
     */
    for (int elementVertexIndex : elementVertexIndices) {
        allVertices[elementVertexIndex] -= *outCenter;
    }
    
    std::shared_ptr<VROData> data = std::make_shared<VROData>(geometrySource->getData()->getData(),
                                                              geometrySource->getData()->getDataLength());
    
    std::shared_ptr<VROGeometrySource> source = std::make_shared<VROGeometrySource>(data, geometrySource);
    source->modifyVertices([&allVertices](int index, VROVector3f vertex) -> VROVector3f {
        return allVertices[index];
    });
    return source->getData();
}

int VROGeometryUtilGetIndicesCount(int primitiveCount, VROGeometryPrimitiveType primitiveType) {
    switch (primitiveType) {
        case VROGeometryPrimitiveType::Triangle:
            return primitiveCount * 3;
            
        case VROGeometryPrimitiveType::TriangleStrip:
            return primitiveCount + 2;
            
        case VROGeometryPrimitiveType::Line:
            return primitiveCount * 2;
            
        case VROGeometryPrimitiveType::Point:
            return primitiveCount;
            
        default:
            break;
    }
}

int VROGeometryUtilGetPrimitiveCount(int indicesCount, VROGeometryPrimitiveType primitiveType) {
    switch (primitiveType) {
        case VROGeometryPrimitiveType::Triangle:
            return indicesCount / 3;
            
        case VROGeometryPrimitiveType::TriangleStrip:
            return indicesCount - 2;
            
        case VROGeometryPrimitiveType::Line:
            return indicesCount / 2;
            
        case VROGeometryPrimitiveType::Point:
            return indicesCount;
            
        default:
            break;
    }
}

int VROGeometryUtilParseAttributeIndex(VROGeometrySourceSemantic semantic) {
    switch (semantic) {
        case VROGeometrySourceSemantic::Vertex:
            return 0;
        case VROGeometrySourceSemantic::Normal:
            return 1;
        case VROGeometrySourceSemantic::Color:
            return 2;
        case VROGeometrySourceSemantic::Texcoord:
            return 3;
        case VROGeometrySourceSemantic::Tangent:
            return 4;
        case VROGeometrySourceSemantic::VertexCrease:
            return 5;
        case VROGeometrySourceSemantic::EdgeCrease:
            return 6;
        case VROGeometrySourceSemantic::BoneWeights:
            return 7;
        case VROGeometrySourceSemantic::BoneIndices:
            return 8;
        case VROGeometrySourceSemantic::Morph_0:
            return 9;
        case VROGeometrySourceSemantic::Morph_1:
            return 10;
        case VROGeometrySourceSemantic::Morph_2:
            return 11;
        case VROGeometrySourceSemantic::Morph_3:
            return 12;
        case VROGeometrySourceSemantic::Morph_4:
            return 13;
        case VROGeometrySourceSemantic::Morph_5:
            return 14;
        case VROGeometrySourceSemantic::Morph_6:
            return 15;
        default:
            return 0;
    }
}
