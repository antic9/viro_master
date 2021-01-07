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

package com.viro.core;

import android.graphics.Color;

/**
 * Light is a light source that illuminates Nodes in the {@link Scene}. Lights can be added to Nodes
 * so they are part of the overall scene graph. The area each Light illuminates depends on its type,
 * which is determined by the subclass of Light used. {@link Spotlight}, for example, illuminates a
 * cone-shaped area; AmbientLight, on the other hand, illuminates everything in the Scene with a
 * constant intensity.
 * <p>
 * The way in which a Light interacts with each surface of a {@link Geometry} to produce a color
 * depends on the materials used by the Geometry, along with the properties of the Light itself.
 * For more information see the Lighting and Materials guide.
 * <p>
 * Lights are performance intensive so it is recommended to use as few as possible, and only to
 * light dynamic objects in the Scene. For static objects, it is better to use pre-generated
 * lighting (e.g. baking the lighting into textures).
 */
public abstract class Light {
    protected long mNativeRef;
    protected long mColor = Color.WHITE;
    protected float mIntensity = 1000;
    private int mInfluenceBitMask = 1;
    private float mTemperature = 6500;

    /**
     * @hide
     */
    protected Light() {

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
     * Release native resources associated with this Light.
     */
    public void dispose() {
        if (mNativeRef != 0) {
            nativeDestroyLight(mNativeRef);
            mNativeRef = 0;
        }
    }

    /**
     * Set the {@link Color} of this Light.
     *
     * @param color The color.
     */
    public void setColor(long color) {
        mColor = color;
        nativeSetColor(mNativeRef, color);
    }

    /**
     * Get the {@link Color} of this Light.
     *
     * @return The color.
     */
    public long getColor() {
        return mColor;
    }

    /**
     * Set the intensity of this Light. Set to 1000 for normal intensity. When using
     * physically-based rendering, this value is specified in Lumens. When using non-physical
     * rendering, the intensity is simply divided by 1000 and multiplied by the Light's color.
     * <p>
     * Lower intensities will decrease the brightness of the light, and higher intensities will
     * increase the brightness of the light.
     *
     * @param intensity The intensity of the Light.
     */
    public void setIntensity(float intensity) {
        mIntensity = intensity;
        nativeSetIntensity(mNativeRef, intensity);
    }

    /**
     * Return the intensity of this Light, which measures its brightness.
     *
     * @return The intensity of the light.
     */
    public float getIntensity() {
        return mIntensity;
    }

    /**
     * Set the temperature of the light, in Kelvin. Viro will derive a hue from this temperature and
     * multiply it by the light's color (which can also be set, via {@link #setColor(long)}). To
     * model a physical light with a known temperature, you can leave color of this Light set to
     * (1.0, 1.0, 1.0) and set its temperature only.
     * <p>
     * The default value for temperature is 6500K, which represents pure white light.
     *
     * @param temperature The temperature in Kelvin.
     */
    public void setTemperature(float temperature) {
        mTemperature = temperature;
        nativeSetTemperature(mNativeRef, temperature);
    }

    /**
     * Get the temperature of the light, in Kelvin.
     *
     * @return The temperature in Kelvin.
     */
    public float getTemperature() {
        return mTemperature;
    }

    /**
     * This property is used to make Lights apply to specific Nodes. Lights and Nodes in the {@link
     * Scene} can be assigned bit-masks to determine how each {@link Light} influences each {@link
     * Node}.
     * <p>
     * During rendering, Viro compares each Light's influenceBitMask with each node's
     * lightReceivingBitMask and shadowCastingBitMask. The bit-masks are compared using a bitwise
     * AND operation:
     * <p>
     * If (influenceBitMask & lightReceivingBitMask) != 0, then the Light will illuminate the Node
     * (and the Node will receive shadows cast from objects occluding the Light).
     * <p>
     * If (influenceBitMask & shadowCastingBitMask) != 0, then the Node will cast shadows from the
     * Light.
     * <p>
     * The default mask is 0x1.
     *
     * @param bitMask The bit mask to set.
     */
    public void setInfluenceBitMask(int bitMask) {
        mInfluenceBitMask = bitMask;
        nativeSetInfluenceBitMask(mNativeRef, bitMask);
    }

    /**
     * Get the influence bit mask for this Light, which determines how it interacts with
     * individual {@link Node}s. See {@link #setInfluenceBitMask(int)} for more information.
     *
     * @return The influence bit mask.
     */
    public int getInfluenceBitMask() {
        return mInfluenceBitMask;
    }

    private native void nativeDestroyLight(long nativeRef);
    private native void nativeSetColor(long lightRef, long color);
    private native void nativeSetIntensity(long lightRef, float intensity);
    private native void nativeSetInfluenceBitMask(long lightRef, int bitMask);
    private native void nativeSetTemperature(long lightRef, float temperature);

// +---------------------------------------------------------------------------+
// | LightBuilder abstract class for Light
// +---------------------------------------------------------------------------+
    /**
     * LightBuilder abstract class for building subclasses of {@link Light}.
     */
    public static abstract class LightBuilder<R extends Light, B extends LightBuilder<R, B>> {
        private R light;

        /**
         * Refer to {@link Light#setInfluenceBitMask(int)}.
         */
        public LightBuilder influenceBitMask(int bitMask) {
            light.setInfluenceBitMask(bitMask);
            return (B) this;
        }

        /**
         * Refer to {@link Light#setInfluenceBitMask(int)}.
         *
         * @return This builder.
         */
        public LightBuilder intensity(float intensity) {
            light.setIntensity(intensity);
            return (B) this;
        }

        /**
         * Refer to Light{@link #setTemperature(float)}.
         *
         * @return This builder.
         */
        public LightBuilder temperature(float temperature) {
            light.setTemperature(temperature);
            return (B) this;
        }

        /**
         * Refer to {@link Light#setColor(long)}.
         *
         * @return This builder.
         */
        public LightBuilder color(long color) {
            light.setColor(color);
            return (B) this;
        }

        /**
         * Returns the built subclass of {@link Light}.
         *
         * @return The built Light.
         */
        public R build() {
            return light;
        }
    }
}
