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

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.opengl.EGL14;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.AttrRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.util.Log;

import com.google.vr.cardboard.ContextUtils;
import com.viro.core.internal.GLSurfaceViewQueue;
import com.viro.core.internal.GLTextureView;
import com.viro.core.internal.PlatformUtil;
import com.viro.core.internal.ViroTouchGestureListener;
import com.viro.renderer.BuildConfig;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;

/**
 * ViroViewScene is a {@link ViroView} for rendering a {@link Scene} on a simple Android View.
 */
public class ViroViewScene extends ViroView {

    private static final String TAG = "Viro";

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("gvr");
        System.loadLibrary("gvr_audio");
        System.loadLibrary("viro_renderer");
    }

    /**
     * Callback interface for responding to {@link ViroViewScene} startup success or failure.
     */
    public interface StartupListener {

        /**
         * Callback invoked when {@link ViroViewScene} has finished initialization, meaning the
         * rendering surface was succesfully created. When this is received, the ViroView is ready
         * to begin rendering content.
         */
        void onSuccess();

        /**
         * Callback invoked when the {@link ViroViewOVR} failed to initialize.
         *
         * @param error        The error code.
         * @param errorMessage The reason for the failure as a string.
         */
        void onFailure(StartupError error, String errorMessage);

    }

    /**
     * Errors returned by the {@link StartupListener}, in response to Viro failing to
     * initialize.
     */
    public enum StartupError {

        /**
         * Indicates an unknown error.
         */
        UNKNOWN,

    };

    private static class ViroSceneRenderer implements GLTextureView.Renderer {
        private WeakReference<ViroViewScene> mView;

        public ViroSceneRenderer(ViroViewScene view) {
            mView = new WeakReference<ViroViewScene>(view);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            ViroViewScene view = mView.get();
            if (view == null) {
                return;
            }

            final int EGL_GL_COLORSPACE_KHR = 0x309D;
            final int EGL_GL_COLORSPACE_SRGB_KHR = 0x3089;

            EGL10 egl = (EGL10) EGLContext.getEGL();
            EGLDisplay display = egl.eglGetCurrentDisplay();
            EGLSurface surface = egl.eglGetCurrentSurface(EGL14.EGL_DRAW);

            int[] value = new int[1];
            boolean sRGBFramebuffer = false;
            egl.eglQuerySurface(display, surface, EGL_GL_COLORSPACE_KHR, value);
            if (value[0] == EGL_GL_COLORSPACE_SRGB_KHR) {
                Log.i(TAG, "Acquired sRGB framebuffer");
                sRGBFramebuffer = true;
            }
            else {
                Log.i(TAG, "Driver reporting sRGB framebuffer *not* acquired [colorspace " + value[0] + "]");
            }

            view.mNativeRenderer.onSurfaceCreated(null);
            view.mNativeRenderer.initializeGL(sRGBFramebuffer);
            if (view.mStartupListener != null) {
                Runnable myRunnable = new Runnable() {
                    @Override
                    public void run() {
                        ViroViewScene view = mView.get();
                        if (view == null || view.mDestroyed) {
                            return;
                        }
                        view.mStartupListener.onSuccess();
                    }
                };
                new Handler(Looper.getMainLooper()).post(myRunnable);
            }
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            ViroViewScene view = mView.get();
            if (view == null) {
                return;
            }
            view.mNativeRenderer.onSurfaceChanged(null, width, height);
        }

        @Override
        public void onDrawFrame(GL10 gl) {
            ViroViewScene view = mView.get();
            if (view == null) {
                return;
            }
            for (FrameListener listener : view.mFrameListeners) {
                listener.onDrawFrame();
            }
            view.mNativeRenderer.drawFrame();
        }
    }

    private static class ViroEGLConfigChooser implements GLTextureView.EGLConfigChooser {

        private boolean mMultisamplingEnabled;

        public ViroEGLConfigChooser(boolean multisamplingEnabled) {
            mMultisamplingEnabled = multisamplingEnabled;
        }

        private int[] getMultisamplingAttributes(int colorBits, int alphaBits, int depthBits, int stencilBits) {
            return new int[] {
                    EGL10.EGL_LEVEL, 0,
                    EGL10.EGL_RENDERABLE_TYPE, 0x00000040,  // EGL_OPENGL_ES3_BIT
                    EGL10.EGL_COLOR_BUFFER_TYPE, EGL10.EGL_RGB_BUFFER,
                    EGL10.EGL_RED_SIZE, colorBits,
                    EGL10.EGL_GREEN_SIZE, colorBits,
                    EGL10.EGL_BLUE_SIZE, colorBits,
                    EGL10.EGL_ALPHA_SIZE, alphaBits,
                    EGL10.EGL_DEPTH_SIZE, depthBits,
                    EGL10.EGL_STENCIL_SIZE, stencilBits,
                    EGL10.EGL_SAMPLE_BUFFERS, 1,
                    EGL10.EGL_SAMPLES, 4,  // This is for 4x MSAA.
                    EGL10.EGL_NONE
            };
        }

        private int[] getBaseAttributes(int colorBits, int alphaBits, int depthBits, int stencilBits) {
            return new int[] {
                    EGL10.EGL_LEVEL, 0,
                    EGL10.EGL_RENDERABLE_TYPE, 0x00000040,  // EGL_OPENGL_ES3_BIT
                    EGL10.EGL_COLOR_BUFFER_TYPE, EGL10.EGL_RGB_BUFFER,
                    EGL10.EGL_RED_SIZE, colorBits,
                    EGL10.EGL_GREEN_SIZE, colorBits,
                    EGL10.EGL_BLUE_SIZE, colorBits,
                    EGL10.EGL_ALPHA_SIZE, alphaBits,
                    EGL10.EGL_DEPTH_SIZE, depthBits,
                    EGL10.EGL_STENCIL_SIZE, stencilBits,
                    EGL10.EGL_NONE
            };
        }

        @Override
        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {
            int colorBits = 8;
            int alphaBits = 8;
            int depthBits = 16;
            int stencilBits = 8;

            int[] attribs = null;
            if (mMultisamplingEnabled) {
                attribs = getMultisamplingAttributes(colorBits, alphaBits, depthBits, stencilBits);
            } else {
                attribs = getBaseAttributes(colorBits, alphaBits, depthBits, stencilBits);
            }

            EGLConfig[] configs = new EGLConfig[1];
            int[] configCounts = new int[1];
            egl.eglChooseConfig(display, attribs, configs, 1, configCounts);

            if (configCounts[0] == 0) {
                // Failed! Error handling.
                return null;
            } else {
                return configs[0];
            }
        }
    }

    private GLTextureView mSurfaceView;
    private AssetManager mAssetManager;
    private List<FrameListener> mFrameListeners = new ArrayList();
    private PlatformUtil mPlatformUtil;
    private boolean mActivityPaused = true;
    private ViroMediaRecorder mMediaRecorder;
    private ViroTouchGestureListener mViroTouchGestureListener;
    private StartupListener mStartupListener;

    /**
     * Create a new ViroViewScene with the default {@link RendererConfiguration}.
     *
     * @param context         The activity context.
     * @param startupListener Listener to respond to startup success or failure. Will be notified of
     *                        any errors encountered while initializing Viro. Optional, may be
     *                        null.
     */
    public ViroViewScene(Context context, StartupListener startupListener) {
        super(context, null);
        init(context, startupListener);
    }

    /**
     * Create a new ViroViewScene with the given {@link RendererConfiguration}, which determines
     * the rendering techniques and rendering fidelity to use for this View.
     *
     * @param context         The activity context.
     * @param startupListener Listener to respond to startup success or failure. Will be notified of
     *                        any errors encountered while initializing Viro. Optional, may be
     *                        null.
     * @param config          The {@link RendererConfiguration} to use.
     */
    public ViroViewScene(Context context, StartupListener startupListener,
                         RendererConfiguration config) {
        super(context, config);
        init(context, startupListener);
    }

    /**
     * @hide
     *
     * @param context
     */
    public ViroViewScene(@NonNull final Context context) {
        this(context, (AttributeSet) null);
    }

    /**
     * @hide
     *
     * @param context
     * @param attrs
     */
    public ViroViewScene(@NonNull final Context context, @Nullable final AttributeSet attrs) {
        this(context, attrs, 0);

    }

    /**
     * @hide
     *
     * @param context
     * @param attrs
     * @param defStyleAttr
     */
    public ViroViewScene(@NonNull final Context context, @Nullable final AttributeSet attrs, @AttrRes final int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        if (ContextUtils.getActivity(context) == null) {
            throw new IllegalArgumentException("An Activity Context is required for Viro functionality.");
        } else {
            init(context, null);
        }
    }

    private void init(Context context, StartupListener rendererStartListener) {
        mSurfaceView = new GLTextureView(context);
        addView(mSurfaceView);

        final Context activityContext = getContext();
        final Activity activity = (Activity) getContext();
        initSurfaceView(rendererStartListener == null);

        mAssetManager = getResources().getAssets();
        mPlatformUtil = new PlatformUtil(
                new GLSurfaceViewQueue(mSurfaceView),
                mFrameListeners,
                activityContext,
                mAssetManager);
        mNativeRenderer = new Renderer(
                getClass().getClassLoader(),
                activityContext.getApplicationContext(), this,
                mAssetManager, mPlatformUtil, mRendererConfig);
        mNativeViroContext = new ViroContext(mNativeRenderer.mNativeRef);
        mStartupListener = rendererStartListener;
        mViroTouchGestureListener = new ViroTouchGestureListener(activity, mNativeRenderer);
        setOnTouchListener(mViroTouchGestureListener);
    }

    /**
     * Used by release tests.
     *
     * @hide
     */
    public void startTests() {
        mSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
    }

    private void initSurfaceView(boolean pauseOnStart) {
        mSurfaceView.setEGLContextClientVersion(3);
        mSurfaceView.setEGLConfigChooser(new ViroEGLConfigChooser(mRendererConfig.isMultisamplingEnabled()));
        mSurfaceView.setPreserveEGLContextOnPause(true);
        mSurfaceView.setEGLWindowSurfaceFactory(new ViroEGLTextureWindowSurfaceFactory());

        mSurfaceView.setRenderer(new ViroSceneRenderer(this));
        if (pauseOnStart) {
            mSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        } else {
            mSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
        }
    }

    /**
     * Set the background color of view. This is the color that will be rendered
     * behind the Viro scenegraph.
     *
     * @param color The {@link android.graphics.Color}'s int value.
     */
    @Override
    public void setBackgroundColor(int color) {
        int alpha  = Color.alpha(color);
        mSurfaceView.setOpaque(alpha == 255);
        mNativeRenderer.setClearColor(color);
    }

    @Override
    protected int getSystemUiVisibilityFlags() {
        return 0;
    }

    /**
     * @hide
     */
    @Override
    public void setDebug(boolean debug) {
        //No-op
    }

    @Override
    public void setScene(Scene scene) {
        if (scene == mCurrentScene) {
            return;
        }
        super.setScene(scene);
        mNativeRenderer.setSceneController(scene.mNativeRef, 0.5f);
    }

    /**
     * @hide
     * @param listener
     */
    @Override
    public void setOnTouchListener(OnTouchListener listener) {
        // If we're adding our own ViroTouchGestureListener, then we add it as the actual
        // touch listener otherwise, we attach the listener to the ViroTouchGestureListener
        // which will forward the touches to the given listener before processing them itself.
        if (listener instanceof ViroTouchGestureListener) {
            super.setOnTouchListener(listener);
        } else if(mViroTouchGestureListener != null) {
            mViroTouchGestureListener.setOnTouchListener(listener);
        }
    }

    /**
     * @hide
     */
    @Override
    public void setVRModeEnabled(boolean vrModeEnabled) {
        //No-op
    }

    /**
     * @hide
     */
    @Override
    public String getPlatform() {
        return "scene";
    }

    /**
     * @hide
     */
    @Override
    public void onActivityPaused(Activity activity) {
        if (mWeakActivity.get() != activity){
            return;
        }

        mActivityPaused = true;
        mNativeRenderer.onPause();
        mSurfaceView.onPause();
    }

    /**
     * @hide
     */
    @Override
    public void onActivityResumed(Activity activity) {
        if (mWeakActivity.get() != activity){
            return;
        }

        mActivityPaused = false;
        changeSystemUiVisibility();
        mNativeRenderer.onResume();
        mSurfaceView.onResume();
    }

    /**
     * @hide
     */
    @Override
    public void onActivityDestroyed(Activity activity) {
        this.dispose();
    }

    @Override
    public void dispose() {
        if (mMediaRecorder != null) {
            mMediaRecorder.dispose();
        }

        if (mViroTouchGestureListener != null) {
            mViroTouchGestureListener.destroy();
        }
        super.dispose();
    }

    /**
     * @hide
     */
    @Override
    public void onActivityCreated(Activity activity, Bundle bundle) {
        //No-op
    }

    /**
     * @hide
     */
    @Override
    public void onActivityStarted(Activity activity) {
        if (mWeakActivity.get() != activity){
            return;
        }

        mNativeRenderer.onStart();
    }

    /**
     * @hide
     */
    @Override
    public void onActivityStopped(Activity activity) {
        if (mWeakActivity.get() != activity){
            return;
        }

        mNativeRenderer.onStop();
    }

    /**
     * @hide
     */
    @Override
    public void onActivitySaveInstanceState(Activity activity, Bundle bundle) {
        // No-op
    }

    /**
     * @hide
     */
    @Override
    public void recenterTracking() {
        // No-op
    }

    @Override
    public ViroMediaRecorder getRecorder() {
        if (mMediaRecorder == null) {
            mMediaRecorder = new ViroMediaRecorder(getContext(), mNativeRenderer,
                    getWidth(), getHeight());
        }
        return mMediaRecorder;
    }

}