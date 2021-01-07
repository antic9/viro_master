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

import java.lang.ref.WeakReference;
import java.util.Stack;

/**
 * AnimationTransaction exposes a simple interface for animating properties of Viro objects. For
 * example, to animate a {@link Node} to a new position, simply enclose the normal {@link
 * Node#setPosition(Vector)} call within an AnimationTransaction:
 * <p>
 * <tt>
 * <pre>
 * AnimationTransaction.begin();
 * AnimationTransaction.setAnimationDuration(5000);
 * node.setPosition(new Vector(-2, 2.5f, -3));
 * AnimationTransaction.commit();</pre>
 * </tt> You can also perform more complex animations, like changing the texture of a Material while
 * rotating a Node. The initial texture will crossfade over to the new texture. We do this in the
 * example below while using an "Ease Out" curve to decelerate the animation as it nears its end.
 * <p>
 * <tt>
 * <pre>
 * AnimationTransaction.begin();
 * AnimationTransaction.setAnimationDuration(5000);
 * AnimationTransaction.setTimingFunction(AnimationTimingFunction.EaseOut);
 *
 * node.setRotation(new Vector(0, M_PI_2, 0));
 * node.getGeometry().getMaterials().get(0).setDiffuseTexture(texture);
 *
 * AnimationTransaction.commit();</pre>
 * </tt> Finally, you can also chain animations or respond to the end of an animation by using a
 * {@link Listener}. In the snippet below, we move the position of a Node then, once that animation
 * completes, we rotate the Node.
 * <p>
 * <tt>
 * <pre>
 * AnimationTransaction.begin();
 * AnimationTransaction.setAnimationDuration(5000);
 * AnimationTransaction.setFinishCallback(new AnimationTransaction.Listener() {
 *     public void onFinished(final AnimationTransaction transaction) {
 *         AnimationTransaction.begin();
 *         node.setRotation(new Vector(0, M_PI_2, 0));
 *         AnimationTransaction.commit();
 *     }
 * });
 * node.setPosition(new Vector(-2, 2.5f, -3));
 * AnimationTransaction.commit();</pre>
 * </tt>
 * <p>
 * For an extended discussion on animation, refer to the <a href="https://virocore.viromedia.com/docs/animation">Animation
 * Guide</a>.
 */
public class AnimationTransaction {

    /**
     * Callback interface for responding to the end of an {@link AnimationTransaction}.
     */
    public interface Listener {
        /**
         * Invoked when an {@link AnimationTransaction} naturally finishes, or when it is
         * forcibly terminated through {@link AnimationTransaction#terminate()}. For looping
         * animations, this callback is invoked on the completion of each loop.
         *
         * @param transaction The transaction that finished.
         */
        public void onFinish(AnimationTransaction transaction);
    }

// +---------------------------------------------------------------------------+
// | STATIC ANIMATION MANAGEMENT
// +---------------------------------------------------------------------------+

    private static Stack<AnimationTransaction> sOpenTransactions = new Stack<AnimationTransaction>();

    /**
     * Begin a new {@link AnimationTransaction}. All animatable properties set on the current
     * thread after this call will be animated according to this new transaction. The animations
     * will commence upon {@link #commit()}.
     */
    public static void begin() {
        long nativeRef = nativeBegin();
        sOpenTransactions.push(new AnimationTransaction(nativeRef));
    }

    /**
     * Commit the {@link AnimationTransaction}, commencing all enclosed animations.
     *
     * @return The {@link AnimationTransaction} corresponding to this transaction.
     */
    public static AnimationTransaction commit() {
        AnimationTransaction transaction = sOpenTransactions.pop();
        nativeCommit(transaction);
        return transaction;
    }

    /**
     * Set the speed at which the animation runs. The default value is 1.0, which runs the animation at normal speed.
     * A value of 0.0 freezes the animation; a value of 2.0 will run the animation at double speed.
     *
     * @param speed A multiplier which represents the speed at which to run the animation. For
     *              example, a value of 1.0 is the default speed. 0.0 freezes the animation, 0.5 is
     *              half speed, and 2.0 is double normal speed.
     */
    public static void setAnimationSpeed(float speed) {
        nativeSetAnimationSpeed(speed);
    }

    /**
     * Set a time offset that indicates from what frame the animation will start. This offset is
     * given in milliseconds from the start of animation. When {@link #commit()} is invoked the
     * animation will start from this time offset.
     *
     * @param timeOffset The time offset in milliseconds.
     */
    public static void setAnimationTimeOffset(long timeOffset) {
        nativeSetAnimationTimeOffset(timeOffset / 1000f);
    }

    /**
     * Set the duration of all animations in this AnimationTransaction, in seconds.
     *
     * @param durationMillis The duration of the transaction, in milliseconds.
     */
    public static void setAnimationDuration(long durationMillis) {
        nativeSetAnimationDuration(durationMillis / 1000f);
    }

    /**
     * Set the time in seconds that we wait before the animations in this transaction start (after
     * the AnimationTransaction is committed).
     *
     * @param delayMillis The milliseconds to delay before beginning the transaction.
     */
    public static void setAnimationDelay(long delayMillis) {
        nativeSetAnimationDelay(delayMillis / 1000f);
    }

    /**
     * Set to true to make this AnimationTransaction automatically loop to the beginning on finish.
     *
     * @param loop True to loop back to the beginning when finished.
     */
    public static void setAnimationLoop(boolean loop) {
        nativeSetAnimationLoop(loop);
    }

    /**
     * Set a {@link Listener} to invoke when the active transaction completes.
     *
     * @param listener The {@link Listener} to use for the current transaction.
     */
    public static void setListener(Listener listener) {
        sOpenTransactions.peek().mListener =  new WeakReference<Listener>(listener);
    }

    /**
     * Set an {@link AnimationTimingFunction}, which defines the pacing of the animation.
     *
     * @param timingFunction The {@link AnimationTimingFunction} to use for the current
     *                       transaction.
     */
    public static void setTimingFunction(AnimationTimingFunction timingFunction) {
        nativeSetTimingFunction(timingFunction.getStringValue());
    }

// +---------------------------------------------------------------------------+
// | ANIMATION TRANSACTION OBJECT
// +---------------------------------------------------------------------------+

    private long mNativeRef;
    private WeakReference<Listener> mListener;

    private AnimationTransaction(long nativeRef) {
        mNativeRef = nativeRef;
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
     * Release native resources associated with this AnimationTransaction.
     */
    public void dispose() {
        if (mNativeRef != 0) {
            nativeDispose(mNativeRef);
            mNativeRef = 0;
        }
    }

    /**
     * Pause this AnimationTransaction. All animations in the transaction will cease updating
     * until {@link #resume()} is invoked. The time remaining will remain in place as well. For
     * example, if the animation is paused with 3 seconds remaining, the animation will have 3
     * seconds remaining when it is resumed.
     */
    public void pause() {
        nativePause(mNativeRef);
    }

    /**
     * Resume a paused AnimationTransaction.
     */
    public void resume() {
        nativeResume(mNativeRef);
    }

    /**
     * Terminate this AnimationTransaction. This will force (snap) all values to their final
     * value, and invoke the {@link Listener}.
     */
    public void terminate() {
        nativeTerminate(mNativeRef);
    }

    private static native long nativeBegin();
    private static native void nativeCommit(AnimationTransaction transaction);
    private static native void nativeSetAnimationSpeed(float speed);
    private static native void nativeSetAnimationDuration(float durationSeconds);
    private static native void nativeSetAnimationDelay(float delaySeconds);
    private static native void nativeSetAnimationTimeOffset(float timeOffset);
    private static native void nativeSetAnimationLoop(boolean loop);
    private static native void nativeSetTimingFunction(String timingFunction);
    private native void nativeDispose(long nativeRef);
    private native void nativePause(long nativeRef);
    private native void nativeResume(long nativeRef);
    private native void nativeTerminate(long nativeRef);

    /*
     Invoked by JNI.
     */
    void onAnimationFinished() {
        Listener listener = mListener == null ? null : mListener.get();
        if (listener != null) {
            listener.onFinish(this);
        }
    }
}
