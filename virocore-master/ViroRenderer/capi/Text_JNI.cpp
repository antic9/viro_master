//
//  Text_JNI.cpp
//  ViroRenderer
//
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

#include <memory>
#include "VROTypeface.h"
#include "VROTypefaceCollection.h"
#include "VROStringUtil.h"
#include "VROText.h"
#include "Node_JNI.h"
#include "TextDelegate_JNI.h"
#include "ViroContext_JNI.h"
#include "VROPlatformUtil.h"

#if VRO_PLATFORM_ANDROID
#define VRO_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_com_viro_core_Text_##method_name
#else
#define VRO_METHOD(return_type, method_name) \
    return_type Text_##method_name
#endif

VROTextHorizontalAlignment getHorizontalAlignmentEnum(const std::string& strName) {
    // Depending on string, return the right enum
    if (VROStringUtil::strcmpinsensitive(strName, "Right")) {
        return VROTextHorizontalAlignment::Right;
    } else if (VROStringUtil::strcmpinsensitive(strName, "Center")) {
        return VROTextHorizontalAlignment::Center;
    } else {
        // Default to left alignment
        return VROTextHorizontalAlignment::Left;
    }

}

VROTextVerticalAlignment getVerticalAlignmentEnum(const std::string& strName) {
    // Depending on string, return the right enum
    if (VROStringUtil::strcmpinsensitive(strName, "Bottom")) {
        return VROTextVerticalAlignment::Bottom;
    } else if (VROStringUtil::strcmpinsensitive(strName, "Center")) {
        return VROTextVerticalAlignment::Center;
    } else {
        // Default to Top alignment
        return VROTextVerticalAlignment::Top;
    }

}

VROLineBreakMode getLineBreakModeEnum(const std::string& strName) {
    // Depending on string, return the right enum
    if (VROStringUtil::strcmpinsensitive(strName, "WordWrap")) {
        return VROLineBreakMode::WordWrap;
    } else if (VROStringUtil::strcmpinsensitive(strName, "CharWrap")) {
        return VROLineBreakMode::CharWrap;
    } else if (VROStringUtil::strcmpinsensitive(strName, "Justify")) {
        return VROLineBreakMode::Justify;
    } else {
        // Default to none
        return VROLineBreakMode::None;
    }
}

VROTextClipMode getTextClipModeEnum(const std::string& strName) {
    // Depending on string, return the right enum
    if (VROStringUtil::strcmpinsensitive(strName, "ClipToBounds")) {
        return VROTextClipMode::ClipToBounds;
    } else {
        return VROTextClipMode::None;
    }
}


VROTextOuterStroke getTextOuterStrokeEnum(const std::string& strName) {
    // Depending on string, return the right enum
    if (VROStringUtil::strcmpinsensitive(strName, "Outline")) {
        return VROTextOuterStroke::Outline;
    } else if (VROStringUtil::strcmpinsensitive(strName, "DropShadow")) {
        return VROTextOuterStroke::DropShadow;
    } else {
        return VROTextOuterStroke::None;
    }
}

extern "C" {

VRO_METHOD(VRO_REF(VROText), nativeCreateText)(VRO_ARGS
                                               VRO_REF(ViroContext) context_j,
                                               VRO_STRING_WIDE text_j,
                                               VRO_STRING fontFamily_j,
                                               VRO_INT size,
                                               VRO_INT style,
                                               VRO_INT weight,
                                               VRO_LONG color,
                                               VRO_FLOAT extrusionDepth,
                                               VRO_STRING outerStroke_j,
                                               VRO_INT outerStrokeWidth_j,
                                               VRO_LONG outerStrokeColor_j,
                                               VRO_FLOAT width,
                                               VRO_FLOAT height,
                                               VRO_STRING horizontalAlignment_j,
                                               VRO_STRING verticalAlignment_j,
                                               VRO_STRING lineBreakMode_j,
                                               VRO_STRING clipMode_j,
                                               VRO_INT maxLines) {
    // Get the text string
    std::wstring text;
    if (!VRO_IS_WIDE_STRING_EMPTY(text_j)) {
        VRO_STRING_GET_CHARS_WIDE(text_j, text);
    }

    // Get the color
    float a = ((color >> 24) & 0xFF) / 255.0;
    float r = ((color >> 16) & 0xFF) / 255.0;
    float g = ((color >> 8) & 0xFF) / 255.0;
    float b = (color & 0xFF) / 255.0;
    VROVector4f vecColor(r, g, b, a);

    // Get horizontal alignment
    VROTextHorizontalAlignment horizontalAlignment = getHorizontalAlignmentEnum(VRO_STRING_STL(horizontalAlignment_j));

    // Get vertical alignment
    VROTextVerticalAlignment verticalAlignment = getVerticalAlignmentEnum(VRO_STRING_STL(verticalAlignment_j));

    // Get line break mode
    VROLineBreakMode lineBreakMode = getLineBreakModeEnum(VRO_STRING_STL(lineBreakMode_j));

    // Get clip mode
    VROTextClipMode clipMode = getTextClipModeEnum(VRO_STRING_STL(clipMode_j));

    // Get the font family
    std::string fontFamily = VRO_STRING_STL(fontFamily_j);

    // Get the outer stroke
    float outerA = ((outerStrokeColor_j >> 24) & 0xFF) / 255.0;
    float outerR = ((outerStrokeColor_j >> 16) & 0xFF) / 255.0;
    float outerG = ((outerStrokeColor_j >> 8) & 0xFF) / 255.0;
    float outerB = (outerStrokeColor_j & 0xFF) / 255.0;
    VROVector4f outerStrokeColor(outerR, outerG, outerB, outerA);

    VROTextOuterStroke outerStroke = getTextOuterStrokeEnum(VRO_STRING_STL(outerStroke_j));

    std::shared_ptr<ViroContext> context = VRO_REF_GET(ViroContext, context_j);
    std::shared_ptr<VRODriver> driver = context->getDriver();

    std::shared_ptr<VROText> vroText = std::make_shared<VROText>(text, fontFamily, size,
                                                                 (VROFontStyle) style,
                                                                 (VROFontWeight) weight,
                                                                 vecColor, extrusionDepth,
                                                                 outerStroke, outerStrokeWidth_j, outerStrokeColor,
                                                                 width, height,
                                                                 horizontalAlignment,
                                                                 verticalAlignment,
                                                                 lineBreakMode,
                                                                 clipMode, maxLines, driver);

    // Update text on renderer thread (glyph creation requires this)
    VROPlatformDispatchAsyncRenderer([vroText] {
        vroText->update();
    });

    return VRO_REF_NEW(VROText, vroText);
}

VRO_METHOD(void, nativeSetText)(VRO_ARGS
                                VRO_REF(VROText) text_j,
                                VRO_STRING_WIDE text_string_j) {

    std::wstring text_string;
    VRO_STRING_GET_CHARS_WIDE(text_string_j, text_string);

    std::weak_ptr<VROText> text_w = VRO_REF_GET(VROText, text_j);
    VROPlatformDispatchAsyncRenderer([text_w, text_string] {
        std::shared_ptr<VROText> text = text_w.lock();
        if (!text) {
            return;
        }
        text->setText(text_string);
    });
}

VRO_METHOD(void, nativeSetFont)(VRO_ARGS
                                VRO_REF(ViroContext) context_j,
                                VRO_REF(VROText) text_j,
                                VRO_STRING family_j,
                                VRO_INT size,
                                VRO_INT style,
                                VRO_INT weight) {
    std::string family = VRO_STRING_STL(family_j);
    std::shared_ptr<ViroContext> context = VRO_REF_GET(ViroContext, context_j);

    std::weak_ptr<VROText> text_w = VRO_REF_GET(VROText, text_j);
    VROPlatformDispatchAsyncRenderer([text_w, family, size, style, weight] {
        std::shared_ptr<VROText> text = text_w.lock();
        if (!text) {
            return;
        }

        text->setTypefaces(family, size, (VROFontStyle) style, (VROFontWeight)weight);
    });
}

VRO_METHOD(void, nativeSetColor)(VRO_ARGS
                                 VRO_REF(VROText) text_j,
                                 VRO_LONG color_j) {

    float a = ((color_j >> 24) & 0xFF) / 255.0;
    float r = ((color_j >> 16) & 0xFF) / 255.0;
    float g = ((color_j >> 8) & 0xFF) / 255.0;
    float b = (color_j & 0xFF) / 255.0;
    VROVector4f color(r, g, b, a);

    std::weak_ptr<VROText> text_w = VRO_REF_GET(VROText, text_j);
    VROPlatformDispatchAsyncRenderer([text_w, color] {
        std::shared_ptr<VROText> text = text_w.lock();
        if (!text) {
            return;
        }
        text->setColor(color);
    });
}

VRO_METHOD(void, nativeSetExtrusionDepth)(VRO_ARGS
                                          VRO_REF(VROText) text_j,
                                          VRO_FLOAT extrusionDepth) {

    std::weak_ptr<VROText> text_w = VRO_REF_GET(VROText, text_j);
    VROPlatformDispatchAsyncRenderer([text_w, extrusionDepth] {
        std::shared_ptr<VROText> text = text_w.lock();
        if (!text) {
            return;
        }
        text->setExtrusion(extrusionDepth);
    });
}

VRO_METHOD(void, nativeSetOuterStroke)(VRO_ARGS
                                       VRO_REF(VROText) text_j,
                                       VRO_STRING stroke_j,
                                       VRO_INT width, VRO_LONG color_j) {
    float a = ((color_j >> 24) & 0xFF) / 255.0;
    float r = ((color_j >> 16) & 0xFF) / 255.0;
    float g = ((color_j >> 8) & 0xFF) / 255.0;
    float b = (color_j & 0xFF) / 255.0;
    VROVector4f color(r, g, b, a);

    VROTextOuterStroke stroke = getTextOuterStrokeEnum(VRO_STRING_STL(stroke_j));

    std::weak_ptr<VROText> text_w = VRO_REF_GET(VROText, text_j);
    VROPlatformDispatchAsyncRenderer([text_w, stroke, width, color] {
        std::shared_ptr<VROText> text = text_w.lock();
        if (!text) {
            return;
        }
        text->setOuterStroke(stroke, width, color);
    });
}

VRO_METHOD(void, nativeSetWidth)(VRO_ARGS
                                 VRO_REF(VROText) text_j,
                                 VRO_FLOAT width) {

    std::weak_ptr<VROText> text_w = VRO_REF_GET(VROText, text_j);
    VROPlatformDispatchAsyncRenderer([text_w, width] {
        std::shared_ptr<VROText> text = text_w.lock();
        if (!text) {
            return;
        }
        text->setWidth(width);
    });
}

VRO_METHOD(void, nativeSetHeight)(VRO_ARGS
                                  VRO_REF(VROText) text_j,
                                  VRO_FLOAT height) {

    std::weak_ptr<VROText> text_w = VRO_REF_GET(VROText, text_j);
    VROPlatformDispatchAsyncRenderer([text_w, height] {
        std::shared_ptr<VROText> text = text_w.lock();
        if (!text) {
            return;
        }
        text->setHeight(height);
    });
}

VRO_METHOD(void, nativeSetHorizontalAlignment)(VRO_ARGS
                                               VRO_REF(VROText) text_j,
                                               VRO_STRING horizontalAlignment_j) {
    VROTextHorizontalAlignment horizontalAlignment
            = getHorizontalAlignmentEnum(VRO_STRING_STL(horizontalAlignment_j));

    std::weak_ptr<VROText> text_w = VRO_REF_GET(VROText, text_j);
    VROPlatformDispatchAsyncRenderer([text_w, horizontalAlignment] {
        std::shared_ptr<VROText> text = text_w.lock();
        if (!text) {
            return;
        }
        text->setHorizontalAlignment(horizontalAlignment);
    });
}

VRO_METHOD(void, nativeSetVerticalAlignment)(VRO_ARGS
                                             VRO_REF(VROText) text_j,
                                             VRO_STRING verticalAlignment_j) {
    VROTextVerticalAlignment verticalAlignment
            = getVerticalAlignmentEnum(VRO_STRING_STL(verticalAlignment_j));

    std::weak_ptr<VROText> text_w = VRO_REF_GET(VROText, text_j);
    VROPlatformDispatchAsyncRenderer([text_w, verticalAlignment] {
        std::shared_ptr<VROText> text = text_w.lock();
        if (!text) {
            return;
        }
        text->setVerticalAlignment(verticalAlignment);
    });
}

VRO_METHOD(void, nativeSetLineBreakMode)(VRO_ARGS
                                         VRO_REF(VROText) text_j,
                                         VRO_STRING lineBreakMode_j) {
    VROLineBreakMode lineBreakMode = getLineBreakModeEnum(VRO_STRING_STL(lineBreakMode_j));

    std::weak_ptr<VROText> text_w = VRO_REF_GET(VROText, text_j);
    VROPlatformDispatchAsyncRenderer([text_w, lineBreakMode] {
        std::shared_ptr<VROText> text = text_w.lock();
        if (!text) {
            return;
        }
        text->setLineBreakMode(lineBreakMode);
    });
}

VRO_METHOD(void, nativeSetClipMode)(VRO_ARGS
                                    VRO_REF(VROText) text_j,
                                    VRO_STRING clipMode_j) {

    // Get clip mode
    VROTextClipMode clipMode = getTextClipModeEnum(VRO_STRING_STL(clipMode_j));

    std::weak_ptr<VROText> text_w = VRO_REF_GET(VROText, text_j);
    VROPlatformDispatchAsyncRenderer([text_w, clipMode] {
        std::shared_ptr<VROText> text = text_w.lock();
        if (!text) {
            return;
        }
        text->setClipMode(clipMode);
    });
}

VRO_METHOD(void, nativeSetMaxLines)(VRO_ARGS
                                    VRO_REF(VROText) text_j,
                                    VRO_INT maxLines) {

    std::weak_ptr<VROText> text_w = VRO_REF_GET(VROText, text_j);
    VROPlatformDispatchAsyncRenderer([text_w, maxLines] {
        std::shared_ptr<VROText> text = text_w.lock();
        if (!text) {
            return;
        }
        text->setMaxLines(maxLines);
    });
}

} // extern "C"

