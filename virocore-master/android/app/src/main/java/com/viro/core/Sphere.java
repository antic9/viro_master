//
//  Copyright (c) 2017-present, ViroMedia, Inc.
//  All rights reserved.
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

/*
 * Java JNI wrapper for linking the following classes below across the bridge.
 *
 * Android Java Object  : com.viromedia.bridge.component.Video360
 * Java JNI Wrapper     : com.viro.renderer.jni.SphereJni.java
 * Cpp JNI wrapper      : Sphere_JNI.cpp
 * Cpp Object           : VROSphere.cpp
 */
package com.viro.core;

/**
 * Sphere defines a spherical shape, e.g. where every point on the surface is equidistant from the
 * center. The center of the Sphere is placed at the origin. The surface of the Sphere is
 * approximated by setHeightSegmentCount and setWidthSegmentCount. These define the number of
 * divisions made along the surface to approximate the sphere.
 */
public class Sphere extends Geometry {

    private float mRadius;
    private int mWidthSegmentCount, mHeightSegmentCount;
    private boolean mFacesOutward;

    /**
     * Construct a new Sphere. The Sphere will use the default segment approximation and will
     * face outward.
     *
     * @param radius The radius of the Sphere.
     */
    public Sphere(float radius) {
        super(false);
        mNativeRef = nativeCreateSphere(radius);
    }

    /**
     * Construct a new, fully parameterized Sphere.
     *
     * @param radius             The radius of the Sphere.
     * @param widthSegmentCount  The number of segments around the circumference of the Sphere.
     * @param heightSegmentCount The number of segments from the south pole to the north pole of the
     *                           Sphere.
     * @param facesOutward       True to have the Sphere face outward.
     */
    public Sphere(float radius, int widthSegmentCount, int heightSegmentCount, boolean facesOutward) {
        super(false);

        mRadius = radius;
        mWidthSegmentCount = widthSegmentCount;
        mHeightSegmentCount = heightSegmentCount;
        mFacesOutward = facesOutward;
        mNativeRef = nativeCreateSphereParameterized(
                radius,
                widthSegmentCount,
                heightSegmentCount,
                facesOutward);
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            dispose();
        } finally {
            super.finalize();
        }
    }

    /**
     * Release native resources associated with this Sphere.
     */
    public void dispose() {
        super.dispose();
    }

    /**
     * Set the radius of the Sphere.
     *
     * @param radius The radius.
     */
    public void setRadius(float radius) {
        mRadius = radius;
        nativeSetRadius(mNativeRef, radius);
    }

    /**
     * Return the radius of the Sphere.
     *
     * @return The radius of the Sphere.
     */
    public float getRadius() {
        return mRadius;
    }

    /**
     * Set the number of segments to use to approximate the Sphere across its latitude.
     * <p>
     * This is the number of segments from the Sphere's south pole to its north pole. Higher segment
     * counts more closely resemble a true sphere, but come at a performance and memory cost.
     *
     * @param heightSegmentCount The number of width segments to use.
     */
    public void setHeightSegmentCount(int heightSegmentCount) {
        mHeightSegmentCount = heightSegmentCount;
        nativeSetHeightSegmentCount(mNativeRef, heightSegmentCount);
    }

    /**
     * Get the number of segments used to approximate the Sphere across its latitude.
     *
     * @return The number of segments.
     */
    public int getHeightSegmentCount() {
        return mHeightSegmentCount;
    }

    /**
     * Set the number of segments to use to approximate the Sphere across its longitude.
     * <p>
     * This is the number of segments across the Sphere's circumference. Higher segment counts more
     * closely resemble a true sphere, but come at a performance and memory cost.
     *
     * @param widthSegmentCount The number of width segments to use.
     */
    public void setWidthSegmentCount(int widthSegmentCount) {
        mWidthSegmentCount = widthSegmentCount;
        nativeSetWidthSegmentCount(mNativeRef, widthSegmentCount);
    }

    /**
     * Get the number of segments used to approximate the Sphere across its longitude.
     *
     * @return The number of segments.
     */
    public int getWidthSegmentCount() {
        return mWidthSegmentCount;
    }

    /**
     * Set whether the Sphere faces outward or inward.
     * <p>
     * Outward is typically used if the Sphere is to be used as a "ball" or similar object in the
     * scene. Inward is used if the Sphere is meant to be a globe enclosing the user (e.g. the user
     * is inside the Sphere).
     *
     * @param facesOutward True to make the Sphere face outward.
     */
    public void setFacesOutward(boolean facesOutward) {
        mFacesOutward = facesOutward;
        nativeSetFacesOutward(mNativeRef, facesOutward);
    }

    /**
     * Return true if the Sphere faces outward, meaning its exterior is visible. If false,
     * then the Sphere's interior will be visible.
     *
     * @return True if the Sphere faces outward.
     */
    public boolean getFacesOutward() {
        return mFacesOutward;
    }

    /*
     Native Functions called into JNI
     */
    private native long nativeCreateSphere(float radius);
    private native long nativeCreateSphereParameterized(float radius, int widthSegmentCount, int heightSegementCount, boolean facesOutward);
    private native void nativeSetWidthSegmentCount(long nodeReference, int widthSegmentCount);
    private native void nativeSetHeightSegmentCount(long nodeReference, int heightSegmentCount);
    private native void nativeSetRadius(long nodeReference, float radius);
    private native void nativeSetFacesOutward(long nodeReference, boolean facesOutward);
    private native void nativeSetVideoTexture(long nodeReference, long textureRef);

    /**
     * @hide
     * @param texture
     */
    //#IFDEF 'viro_react'
    public void setVideoTexture(VideoTexture texture){
        nativeSetVideoTexture(mNativeRef, texture.mNativeRef);
    }
    //#ENDIF

}
