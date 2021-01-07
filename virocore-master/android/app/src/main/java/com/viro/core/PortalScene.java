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

import android.graphics.Bitmap;
import android.net.Uri;

/**
 * PortalScene is the root of the subgraph of {@link Node}s that is displayed through a {@link
 * Portal}. Each PortalScene can contain any number of child nodes and content, and each PortalScene
 * can have its own background texture. If a PortalScene is set to <tt>passable</tt>, users are able
 * to walk <i>through</i> the {@link Portal} into the PortalScene.
 * <p>
 * PortalScenes are typically used in AR to create an effect where the user walks from the real
 * world into a virtual world.
 *
 * @see Portal
 */
public class PortalScene extends Node {

    /**
     * Callback interface for responding to users entering or exiting a PortalScene.
     */
    public interface EntryListener {
        /**
         * Invoked when the user enters this PortalScene.
         *
         * @param portalScene The PortalScene entered.
         */
        void onPortalEnter(PortalScene portalScene);

        /**
         * Invoked when the user exits this PortalScene.
         *
         * @param portalScene The PortalScene exited.
         */
        void onPortalExit(PortalScene portalScene);
    }

    private long mNativeDelegateRef;
    private EntryListener mEntryListener = null;
    private boolean mPassable = false;
    private Portal mPortal;
    private Texture mLightingEnvironment;
    private Texture mBackgroundTexture;
    private static final long INVALID_REF = 0;

    /**
     * Construct a new PortalScene. To be displayed, a {@link Portal} entrance has to be set for
     * this PortalScene via {@link #setPortalEntrance(Portal)}. The user will then be able to see
     * into this PortalScene through the entrance.
     */
    public PortalScene() {
        super(false);
        initWithNativeRef(nativeCreatePortalScene());
        attachDelegate();
    }

    /**
     * Used by Scene constructor for the root PortalScene.
     * @hide
     */
    PortalScene(boolean dummyArg) {
        super(dummyArg);
    }

    /**
     * Used by Scene constructor for the root PortalScene.
     * @hide
     */
    void attachDelegate() {
        mNativeDelegateRef = nativeCreatePortalDelegate();
        nativeAttachDelegate(mNativeRef, mNativeDelegateRef);
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
     * Release native resources associated with this PortalScene.
     */
    public void dispose() {
        if (mNativeRef != 0) {
            nativeDestroyPortalScene(mNativeRef);
            mNativeRef = 0;
        }

        if (mNativeDelegateRef != 0) {
            nativeDestroyPortalDelegate(mNativeDelegateRef);
            mNativeDelegateRef = 0;
        }

        if(mBackgroundTexture != null) {
            mBackgroundTexture.dispose();
            mBackgroundTexture = null;
        }
    }

    /**
     * Set this PortalScene to be passable, meaning the user can walk into it. If true, the Portal
     * serves as a 'door' into the world encompassed by this PortalScene. If false, the
     * Portal serves as a 'window' into a world that can't be accessed.
     *
     * @param passable True to make the PortalScene passable.
     */
    public void setPassable(boolean passable) {
        mPassable = passable;
        nativeSetPassable(mNativeRef, passable);
    }

    /**
     * Return true if this PortalScene can be entered.
     *
     * @return True if the PortalScene is passable.
     */
    public boolean isPassable() {
        return mPassable;
    }

    /**
     * Set the {@link Portal} that will serve as an entrance into this PortalScene. The Portal
     * is essentially a {@link Node} containing {@link Geometry} or children Nodes that represent
     * the <i>entrance</i> to this PortalScene. The Portal's Geometry should be aligned with the
     * X axis in its local coordinate system, orthogonal to the Z axis, in order to display properly
     * as a gateway into the PortalScene. The transparent sections of the Portal will render the
     * PortalScene; the non-transparent sections will render normally; and the area outside the
     * Portal will render the current Scene that contains the user.
     *
     * @param portal The {@link Portal} to use as the entrance.
     */
    public void setPortalEntrance(Portal portal) {
        mPortal = portal;
        nativeSetPortalEntrance(mNativeRef, portal.mNativeRef);
    }

    /**
     * Get the {@link Portal} that serves as this PortalScene's entrance, if any.
     *
     * @return The entrance Portal for this PortalScene.
     */
    public Portal getPortalEntrance() {
        return mPortal;
    }

    /**
     * Set the {@link EntryListener} to use for responding to the user entering or exiting this
     * {@link PortalScene}. Only used if this PortalScene is set to passable.
     *
     * @param delegate The listener to use with this PortalScene.
     */
    public void setEntryListener(EntryListener delegate) {
        mEntryListener = delegate;
    }

    /**
     * Get the {@link EntryListener} used for responding to portal entry events.
     *
     * @return The listener, or null if none is set.
     */
    public EntryListener getEntryListener() {
        return mEntryListener;
    }

    /**
     * Set the background of this PortalScene to display the texture. The provided {@link Texture} should
     * contain a spherical image or spherical video. The image or video will be rendered behind all other
     * content.
     *
     * @param texture The {@link Texture} containing the image, or {@link VideoTexture} containing
     *                     the video.
     */
    public void setBackgroundTexture(Texture texture) {
        mBackgroundTexture = texture;
        nativeSetBackgroundTexture(mNativeRef, texture.mNativeRef);
    }

    /**
     * Rotate the background image or video by the given radians about the X, Y, and Z axes.
     *
     * @param rotation {@link Vector} containing rotation about the X, Y, and Z axes in radians.
     */
    public void setBackgroundRotation(Vector rotation) {
        nativeSetBackgroundRotation(mNativeRef, rotation.x, rotation.y, rotation.z);
    }

    /**
     * Set the background of this PortalScene to display a cube-map. The provided {@link Texture}
     * should wrap a cube-map image. The cube-map will be rendered behind all other
     * content.
     *
     * @param cubeTexture The {@link Texture} containing the cube-map.
     */
    public void setBackgroundCubeTexture(Texture cubeTexture) {
        nativeSetBackgroundCubeImageTexture(mNativeRef, cubeTexture.mNativeRef);
    }

    /**
     * Set the background of this PortalScene to display the given color. The color will be rendered
     * behind all other content.
     *
     * @param color The {@link android.graphics.Color}'s int value.
     */
    public void setBackgroundCubeWithColor(long color) {
        nativeSetBackgroundCubeWithColor(mNativeRef, color);
    }

    /**
     * Set the lighting environment to use for the PortalScene. The lighting environment is a {@link
     * Texture} that acts as a global light source, illuminating surfaces with diffuse and specular
     * ambient light. Each pixel in the lighting environment is treated as a light emitter, thereby
     * capturing the environment's global lighting and general feel. This gives objects a sense of
     * belonging to their environment. For this reason it is common to use the scene's background
     * texture as the lighting environment, but this is not necessary.
     * <p>
     * This method expects an equirectangular texture. Radiance HDR textures (*.hdr) work best,
     * and can be loaded with {@link Texture#loadRadianceHDRTexture(Uri)}.
     *
     * @param lightingEnvironment The equirectangular {@link Texture} to use as the lighting
     *                            environment.
     */
    public void setLightingEnvironment(Texture lightingEnvironment) {
        mLightingEnvironment = lightingEnvironment;
        long ref = lightingEnvironment == null ? INVALID_REF : lightingEnvironment.mNativeRef;
        nativeSetLightingEnvironment(mNativeRef, ref);
    }

    /**
     * Get the lighting environment used by this Scene. The lighting environment is a {@link
     * Texture} that acts as a global light source, illuminating surfaces with diffuse and specular
     * ambient light.
     *
     * @return The equirectangular {@link Texture} being used as the lighting environment.
     */
    public Texture getLightingEnvironment() {
        return mLightingEnvironment;
    }

    private native long nativeCreatePortalScene();
    private native long nativeCreatePortalDelegate();
    private native long nativeDestroyPortalScene(long nativeRef);
    private native long nativeDestroyPortalDelegate(long delegateRef);
    private native void nativeSetBackgroundTexture(long nativeRef, long imageRef);
    private native void nativeSetBackgroundCubeImageTexture(long nativeRef, long textureRef);
    private native void nativeSetBackgroundCubeWithColor(long nativeRef, long color);
    private native void nativeSetBackgroundRotation(long nativeRef, float radiansX, float radiansY,
                                                    float radiansZ);
    private native void nativeSetLightingEnvironment(long sceneRef, long textureRef);
    private native long nativeAttachDelegate(long nativeRef, long delegateRef);
    private native void nativeSetPassable(long nativeRef, boolean passable);
    private native void nativeSetPortalEntrance(long nativeRef, long portalNativeRef);

    /*
     Invoked from JNI.
     */
    /**
     * @hide
     */
    void onPortalEnter() {
        if (mEntryListener != null && mNativeRef != 0) {
            mEntryListener.onPortalEnter(this);
        }
    }

    /**
     * @hide
     */
    void onPortalExit() {
        if (mEntryListener != null && mNativeRef != 0) {
            mEntryListener.onPortalExit(this);
        }
    }

// +---------------------------------------------------------------------------+
// | PortalSceneBuilder class for PortalScene
// +---------------------------------------------------------------------------+

    /**
     * PortalSceneBuilder for creating {@link PortalScene} objects.
     */
    public static class PortalSceneBuilder<R extends Node, B extends PortalSceneBuilder<R,B>> {
        private PortalScene portalScene;

        /**
         * Constructor for PortalSceneBuilder class.
         */
        public PortalSceneBuilder() {
            portalScene = new PortalScene();
        }

        /**
         * Refer to {@link PortalScene#setPassable(boolean)}.
         *
         * @return This builder.
         */
        public PortalSceneBuilder passable(boolean passable) {
            portalScene.setPassable(passable);
            return (B) this;
        }

        /**
         * Refer to {@link PortalScene#setPortalEntrance(Portal)}.
         *
         * @return This builder.
         */
        public PortalSceneBuilder portalEntrance(Portal portal) {
            portalScene.setPortalEntrance(portal);
            return (B) this;
        }

        /**
         * Refer to {@link PortalScene#setEntryListener(EntryListener)}.
         *
         * @return This builder.
         */
        public PortalSceneBuilder entryListener(EntryListener delegate) {
            portalScene.setEntryListener(delegate);
            return (B) this;
        }

        /**
         * Refer to {@link PortalScene#setBackgroundTexture(Texture)}.
         *
         * @return This builder.
         */
        public PortalSceneBuilder backgroundTexture(Texture texture) {
            portalScene.setBackgroundTexture(texture);
            return (B) this;
        }

        /**
         * Refer to {@link PortalScene#setBackgroundRotation(Vector)}.
         *
         * @return This builder.
         */
        public PortalSceneBuilder backgroundRotation(Vector rotation){
            portalScene.setBackgroundRotation(rotation);
            return (B) this;
        }

        /**
         * Refer to {@link PortalScene#setBackgroundCubeTexture(Texture)}.
         *
         * @return This builder.
         */
        public PortalSceneBuilder backgroundCubeTexture(Texture cubeTexture) {
            portalScene.setBackgroundCubeTexture(cubeTexture);
            return (B) this;
        }

        /**
         * Refer to {@link PortalScene#setBackgroundCubeWithColor(long)}.
         *
         * @return This builder.
         */
        public PortalSceneBuilder backgroundCubeWithColor(long color) {
            portalScene.setBackgroundCubeWithColor(color);
            return (B) this;
        }

        /**
         * Refer to {@link PortalScene#setLightingEnvironment(Texture)}.
         *
         * @return This builder.
         */
        public PortalSceneBuilder lightingEnvironment(Texture lightingEnvironment) {
            portalScene.setLightingEnvironment(lightingEnvironment);
            return (B) this;
        }
    }
}
