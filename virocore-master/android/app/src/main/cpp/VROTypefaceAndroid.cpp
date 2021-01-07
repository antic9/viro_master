//
//  VROTypefaceAndroid.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 11/24/16.
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

#include "VROTypefaceAndroid.h"
#include "VROLog.h"
#include "VROGlyphOpenGL.h"
#include "VRODriverOpenGLAndroid.h"

static const std::string kSystemFont = "Roboto-Regular";

VROTypefaceAndroid::VROTypefaceAndroid(std::string name, std::string file, int index, int size,
                                       VROFontStyle style, VROFontWeight weight, std::shared_ptr<VRODriver> driver) :
    VROTypeface(name, size, style, weight),
    _driver(driver),
    _face(nullptr),
    _file(file),
    _index(index),
    _numFaces(1) {

}

VROTypefaceAndroid::~VROTypefaceAndroid() {
    std::shared_ptr<VRODriver> driver = _driver.lock();
    if (driver && _face != nullptr) {
        // FT crashes if we delete a face after the freetype library has been deleted
        if (std::dynamic_pointer_cast<VRODriverOpenGLAndroid>(driver)->getFreetype() != nullptr) {
            FT_Done_Face(_face);
        }
    }
}

FT_FaceRec_ *VROTypefaceAndroid::loadFTFace() {
    std::shared_ptr<VRODriver> driver = _driver.lock();
    if (!driver) {
        return nullptr;
    }
    std::string name = getName();

    FT_Library ft = std::dynamic_pointer_cast<VRODriverOpenGLAndroid>(driver)->getFreetype();
    pinfo("Loading font face [name: %s, index: %d]", name.c_str(), _index);

    if (_index == -1) {
        pinfo("Failed to find suitable face matching [%s], defaulting to system font", name.c_str());
        if (FT_New_Face(ft, getFontPath(kSystemFont, "ttf").c_str(), 0, &_face)) {
            pabort("Failed to load system font %s", kSystemFont.c_str());
        }
    }
    else {
        //pinfo("Found path %s and index %d for desired typeface %s with style %d and weight %d",
        //        fileAndIndex.first.c_str(), fileAndIndex.second, name.c_str(), (int) getStyle(), (int) getWeight());
        if (FT_New_Face(ft, _file.c_str(), _index, &_face)) {
            pinfo("Failed to load font face [%s], defaulting to system font", name.c_str());
            if (FT_New_Face(ft, getFontPath(kSystemFont, "ttf").c_str(), 0, &_face)) {
                pabort("Failed to load system font %s", kSystemFont.c_str());
            }
        }
    }
    FT_Set_Pixel_Sizes(_face, 0, getSize());
    _numFaces = _face->num_faces;

    return _face;
}

std::shared_ptr<VROGlyph> VROTypefaceAndroid::loadGlyph(uint32_t charCode, uint32_t variantSelector,
                                                        uint32_t outlineWidth, VROGlyphRenderMode renderMode) {
    std::shared_ptr<VROGlyph> glyph = std::make_shared<VROGlyphOpenGL>();
    std::shared_ptr<VRODriverOpenGLAndroid> driver = std::dynamic_pointer_cast<VRODriverOpenGLAndroid>(_driver.lock());
    if (!driver) {
        return glyph;
    }

    if (renderMode == VROGlyphRenderMode::None) {
        glyph->loadMetrics(_face, charCode, variantSelector);
    } else if (renderMode == VROGlyphRenderMode::Bitmap) {
        glyph->loadBitmap(_face, charCode, variantSelector, &_glyphAtlases, driver);
        if (outlineWidth > 0) {
            glyph->loadOutlineBitmap(driver->getFreetype(), _face, charCode, variantSelector, outlineWidth,
                                     &_outlineAtlases[outlineWidth], driver);
        }
    } else {
        glyph->loadVector(_face, charCode, variantSelector);
    }

    return glyph;
}

std::string VROTypefaceAndroid::getFontPath(std::string fontName, std::string suffix) {
    std::string prefix = "/system/fonts/";
    return prefix + fontName + "." + suffix;
}

float VROTypefaceAndroid::getLineHeight() const {
    return _face->size->metrics.height >> 6;
}