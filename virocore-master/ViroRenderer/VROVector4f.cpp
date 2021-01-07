//
//  VROVector4f.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 11/30/15.
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

#include "VROVector4f.h"
#include <sstream>

VROVector4f::VROVector4f() {
    x = 0;
    y = 0;
    z = 0;
    w = 1;
}

VROVector4f::VROVector4f(const float *components, int count) {
    x = count > 0 ? components[0] : 0;
    y = count > 1 ? components[1] : 0;
    z = count > 2 ? components[2] : 0;
    w = count > 3 ? components[3] : 1;
}

VROVector4f::VROVector4f(float xIn, float yIn, float zIn, float wIn) :
    x(xIn), y(yIn), z(zIn), w(wIn) {
    
}

VROVector4f::VROVector4f(const VROVector4f &vector) :
    x(vector.x),
    y(vector.y),
    z(vector.z),
    w(vector.w)
{}

VROVector4f::~VROVector4f() {
    
}

int VROVector4f::hash() const {
    return floor(x + 31 * y + 31 * z + 121 * w);
}

bool VROVector4f::isEqual(const VROVector4f &vertex) const {
    return fabs(x - vertex.x) < .00001 &&
           fabs(y - vertex.y) < .00001 &&
           fabs(z - vertex.z) < .00001 &&
           fabs(w - vertex.w) < .00001;
}

void VROVector4f::clear() {
    x = 0;
    y = 0;
    z = 0;
    w = 0;
}

void VROVector4f::set(const VROVector4f &value) {
    x = value.x;
    y = value.y;
    z = value.z;
    w = value.w;
}

void VROVector4f::set(float x, float y, float z, float w) {
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}

float VROVector4f::dot(const VROVector4f &vB) const {
    return x * vB.x + y * vB.y + z * vB.z + w * vB.w;
}

void VROVector4f::add(const VROVector4f &vB, VROVector4f *result) const {
    result->x = x + vB.x;
    result->y = y + vB.y;
    result->z = z + vB.z;
    result->w = w + vB.w;
}

void VROVector4f::addScaled(const VROVector4f &scaledB, float scale, VROVector4f *result) const {
    result->x = x + scaledB.x * scale;
    result->y = y + scaledB.y * scale;
    result->z = z + scaledB.z * scale;
    result->w = w + scaledB.w * scale;
}

void VROVector4f::subtract(const VROVector4f &vB, VROVector4f *result) const {
    result->x = x - vB.x;
    result->y = y - vB.y;
    result->z = z - vB.z;
    result->w = w - vB.w;
}

void VROVector4f::midpoint(const VROVector4f &other, VROVector4f *result) const {
    result->x = (x + other.x) * 0.5f;
    result->y = (y + other.y) * 0.5f;
    result->z = (z + other.z) * 0.5f;
    result->w = (w + other.w) * 0.5f;
}

VROVector4f VROVector4f::normalize() const {
    float inverseMag = 1.0f / sqrtf(x * x + y * y + z * z + w * w);
    
    VROVector4f result;
    result.x = x * inverseMag;
    result.y = y * inverseMag;
    result.z = z * inverseMag;
    result.w = w * inverseMag;
    
    return result;
}

float VROVector4f::magnitude() const {
    return sqrt(x * x + y * y + z * z + w * w);
}

bool VROVector4f::isZero() const {
    return x == 0 && y == 0 && z == 0 && w == 0;
}

void VROVector4f::scale(float factor, VROVector4f *result) const {
    result->x = x * factor;
    result->y = y * factor;
    result->z = z * factor;
    result->w = w * factor;
}

VROVector4f VROVector4f::interpolate(VROVector4f other, float t) {
    VROVector4f result;
    result.x = x + (other.x - x) * t;
    result.y = y + (other.y - y) * t;
    result.z = z + (other.z - z) * t;
    result.w = w + (other.w - w) * t;
    
    return result;
}

std::string VROVector4f::toString() const {
    std::stringstream ss;
    ss << "[x: " << x << ", y: " << y << ", z: " << z << "]";
    
    return ss.str();
}
