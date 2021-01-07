//
//  VROVROVectorizer.h
//  ViroKit
//
//  Created by Raj Advani on 7/19/18.
//  Copyright © 2018 Viro Media. All rights reserved.
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
//  Modifed from FTGL library
/*
 * FTGL - OpenGL font library
 *
 * Copyright (c) 2001-2004 Henry Maddocks <ftgl@opengl.geek.nz>
 * Copyright (c) 2008 Sam Hocevar <sam@zoy.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _DIFONT_VECTORISER_H_
#define _DIFONT_VECTORISER_H_

#include <list>
#include <vector>
#include "VROContour.h"

const double FTGL_FRONT_FACING = 1.0;
const double FTGL_BACK_FACING = -1.0;

/**
 * VROVectorizer class is a helper class that converts font outlines into
 * point data.
 *
 * @see ExtrudeGlyph
 * @see OutlineGlyph
 * @see PolygonGlyph
 * @see Contour
 * @see Point
 *
 */
class VROVectorizer {
    public:
        /**
         * Constructor
         *
         * @param glyph The freetype glyph to be processed
         */
        VROVectorizer(const FT_GlyphSlot glyph, unsigned short bezierSteps);

        /**
         *  Destructor
         */
        virtual ~VROVectorizer();

        /**
         * Get the total count of points in this outline
         *
         * @return the number of points
         */
        size_t getPointCount();

        /**
         * Get the count of contours in this outline
         *
         * @return the number of contours
         */
        size_t getContourCount() const { return ftContourCount; }

        /**
         * Return a contour at index
         *
         * @return the number of contours
         */
        const VROContour *const getContour(size_t index) const;

        /**
         * Get the number of points in a specific contour in this outline
         *
         * @param c     The contour index
         * @return      the number of points in contour[c]
         */
        size_t getContourSize(int c) const { return contourList[c]->getPointCount(); }

        /**
         * Get the flag for the tesselation rule for this outline
         *
         * @return The contour flag
         */
        int getContourFlag() const { return contourFlag; }

    private:
    
        /**
         * Process the freetype outline data into contours of points
         *
         * @param front front outset distance
         * @param back back outset distance
         */
        void processContours(unsigned short bezierSteps);

        /**
         * The list of contours in the glyph
         */
        VROContour **contourList;

        /**
         * The number of contours reported by Freetype
         */
        short ftContourCount;
        
        /**
         * A flag indicating the tesselation rule for the glyph
         */
        int contourFlag;
        
        /**
         * A Freetype outline
         */
        FT_Outline outline;
};

#endif  //  __VROVectorizer__
