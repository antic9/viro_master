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

/**
 * RendererConfiguration allows you to set which rendering properties and algorithms will be
 * enabled at launch. These properties may also be dynamically turned on or off via the
 * {@link ViroView}.
 */
public class RendererConfiguration {

    private boolean mShadowsEnabled = true;
    private boolean mHDREnabled = true;
    private boolean mPBREnabled = true;
    private boolean mBloomEnabled = true;
    private boolean mMultisamplingEnabled = false;

    /**
     * Create a new default RendererConfiguration. The default RendererConfiguration enables
     * the highest fidelity rendering supported by the current device: shadows, HDR, PBR, and
     * bloom.
     */
    public RendererConfiguration() {

    }

    /**
     * Enable or disable rendering dynamic shadows. If shadows are disabled here, shadow
     * casting {@link Light}s will simply not cast a shadow.
     * <p>
     * Shadows are not supported on all devices.
     *
     * @param enabled True to enable dynamic shadows for this configuration.
     */
    public void setShadowsEnabled(boolean enabled) {
        mShadowsEnabled = enabled;
    }

    /**
     * Returns true if dynamic shadows are enabled for this configuration.
     *
     * @return True if shadows are enabled.
     */
    public boolean isShadowsEnabled() {
        return mShadowsEnabled;
    }

    /**
     * When HDR rendering is enabled, Viro uses a deeper color space and renders to a floating point
     * texture, where a tone-mapping algorithm is applied to preserve fine details in both bright
     * and dark regions of the scene. If HDR is disabled, then features like Bloom and PBR will not
     * work, and tone-mapping will be disabled.
     * <p>
     * HDR is not supported on all devices.
     *
     * @param enabled True to enable HDR for this configuration.
     */
    public void setHDREnabled(boolean enabled) {
        mHDREnabled = enabled;
    }

    /**
     * Returns true if HDR rendering is enabled for this configuration.
     *
     * @return True if HDR is enabled.
     */
    public boolean isHDREnabled() {
        return mHDREnabled;
    }

    /**
     * Enable or disable physically-based rendering. Physically based rendering, or PBR, produces
     * more realistic lighting results for your scenes, and provides a more intuitive workflow for
     * artists. To use PBR, this property must be enabled, and {@link Material}s must use the {@link
     * com.viro.core.Material.LightingModel#PHYSICALLY_BASED} lighting model. PBR is controlled by a
     * variety of properties, see {@link com.viro.core.Material.LightingModel#PHYSICALLY_BASED} for
     * details.
     *
     * @param enabled True to enable PBR for this configuration.
     */
    public void setPBREnabled(boolean enabled) {
        mPBREnabled = enabled;
    }

    /**
     * Returns true if physically-based rendering is enabled for this configuration.
     *
     * @return True if PBR is enabled.
     */
    public boolean isPBREnabled() {
        return mPBREnabled;
    }

    /**
     * Enable or disable bloom. Bloom adds a soft glow to bright areas in scene, simulating the way
     * bright highlights appear to the human eye. To make an object bloom, this property must be
     * enabled, and the objects's threshold for bloom must be set via {@link Material#setBloomThreshold(float)}.
     *
     * @param enabled True to enable bloom for this configuration.
     */
    public void setBloomEnabled(boolean enabled) {
        mBloomEnabled = enabled;
    }

    /**
     * Returns true if bloom is enabled for this configuration.
     *
     * @return True if bloom is enabled.
     */
    public boolean isBloomEnabled() {
        return mBloomEnabled;
    }

    /**
     * Set to true to enable 4x full-screen multisampling. This helps reduce jagged edges in
     * geometry, but takes <i>significantly</i> more GPU processing time and will drain battery
     * at a faster rate. This defaults to false and is not recommended except on low resolution
     * devices.
     *
     * @param enabled True to enable 4x multisampling.
     */
    public void setMultisamplingEnabled(boolean enabled) { mMultisamplingEnabled = enabled; }

    /**
     * Return true if 4x multisampling is enabled for this configuration.
     *
     * @return True if 4x multisampling is enabled.
     */
    public boolean isMultisamplingEnabled() { return mMultisamplingEnabled; }

}
