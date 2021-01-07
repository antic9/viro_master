//
//  VROGeometrySubstrateMetal.h
//  ViroRenderer
//
//  Created by Raj Advani on 11/18/15.
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

#ifndef VROGeometrySubstrateMetal_h
#define VROGeometrySubstrateMetal_h

#include "VRODefines.h"
#if VRO_METAL

#include "VROGeometrySubstrate.h"
#include "VROGeometrySource.h"
#include "VROGeometryElement.h"
#include <vector>
#include <memory>
#include <map>
#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>

class VROGeometry;
class VROMaterial;
class VROGeometrySource;
class VROGeometryElement;
class VRORenderContext;
class VRODriver;
class VRODriverMetal;
class VROMaterialSubstrateMetal;
class VROConcurrentBuffer;

struct VROVertexArrayMetal {
    id <MTLBuffer> buffer;
};

struct VROGeometryElementMetal {
    id <MTLBuffer> buffer;
    MTLPrimitiveType primitiveType;
    int indexCount;
    MTLIndexType indexType;
    int indexBufferOffset;
};

/*
 Metal representation of a VROGeometry. 
 
 Each set of VROGeometrySources that share the same underlying data buffer are combined
 together into a single VROGeometrySourceMetal, which contains a single MTLBuffer,
 and single MTLVertexDescriptor that describes the interleaved data between the geometry
 sources. At rendering time, all of these vertex buffers are attached to the render encoder
 in successive indexes, starting at 0, via [renderEncoder setVertexBuffer:offset:atIndex:].
 
 For each VROGeometryElement, we create a VROGeometryElementMetal, which contains all the
 information necessary for [renderEncoder drawIndexedPrimitives] using the attached 
 vertex buffers.
 */
class VROGeometrySubstrateMetal : public VROGeometrySubstrate {
    
public:
    
    VROGeometrySubstrateMetal(const VROGeometry &geometry,
                              VRODriverMetal &driver);
    virtual ~VROGeometrySubstrateMetal();
    
    void render(const VROGeometry &geometry,
                int elementIndex,
                VROMatrix4f transform,
                VROMatrix4f normalMatrix,
                float opacity,
                std::shared_ptr<VROMaterial> &material,
                const VRORenderContext &context,
                std::shared_ptr<VRODriver> &driver);
    
private:
    
    MTLVertexDescriptor *_vertexDescriptor;
    VROVertexArrayMetal _var;
    std::vector<VROGeometryElementMetal> _elements;
    
    /*
     Pipeline and depth states for each geometry element. Note that pipeline 
     state is determined by both the geometry (by way of the _vertexDescriptor) 
     and the material; this is why it's not a member of the VROMaterialSubstrate.
     */
    std::vector<id <MTLRenderPipelineState>> _elementPipelineStates;
    std::vector<id <MTLDepthStencilState>> _elementDepthStates;
    
    /*
     Uniforms for the view.
     */
    VROConcurrentBuffer *_viewUniformsBuffer;
    
    /*
     Parse the given geometry elements and populate the _elements vector with the
     results.
     */
    void readGeometryElements(id <MTLDevice> device,
                              const std::vector<std::shared_ptr<VROGeometryElement>> &elements);
    
    /*
     Parse the given geometry sources and populate the _vars vector with the
     results.
     */
    void readGeometrySources(id <MTLDevice> device,
                             const std::vector<std::shared_ptr<VROGeometrySource>> &sources);
    
    /*
     Update the render pipeline state and depth-stencil pipeline state in response to materials
     changing.
     */
    void updatePipelineStates(const VROGeometry &geometry,
                              VRODriverMetal &driver);
    
    /*
     Create a pipeline state from the given material, using the current _vertexDescriptor.
     */
    id <MTLRenderPipelineState> createRenderPipelineState(const std::shared_ptr<VROMaterial> &material,
                                                          VRODriverMetal &driver);
    
    /*
     Create a depth/stencil state from the given material.
     */
    id <MTLDepthStencilState> createDepthStencilState(const std::shared_ptr<VROMaterial> &material,
                                                      id <MTLDevice> device);
    
    /*
     Parse an MTLVertexFormat from the given geometry source.
     */
    MTLVertexFormat parseVertexFormat(std::shared_ptr<VROGeometrySource> &source);
    
    /*
     Parse an MTLPrimitiveType from the given geometry VROGeometryPrimitiveType.
     */
    MTLPrimitiveType parsePrimitiveType(VROGeometryPrimitiveType primitive);
    
    /*
     Rendering helper function.
     */
    void renderMaterial(VROMaterialSubstrateMetal *material,
                        VROGeometryElementMetal &element,
                        id <MTLRenderPipelineState> pipelineState,
                        id <MTLDepthStencilState> depthStencilState,
                        id <MTLRenderCommandEncoder> renderEncoder,
                        float opacity,
                        const VRORenderContext &renderContext,
                        std::shared_ptr<VRODriver> &driver);
    
};

#endif
#endif /* VROGeometrySubstrateMetal_h */
