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

/**
 * The PhysicsWorld encapsulates the physics simulation associated with the {@link Scene}. Each
 * Scene automatically creates a PhysicsWorld upon construction. This object can be used to set
 * global physics properties like gravity.
 * <p>
 * For an extended discussion of physics in Viro, refer to the <a href="https://virocore.viromedia.com/docs/physics">Physics
 * Guide</a>.
 */
public class PhysicsWorld {

    /**
     * Callback used when hit tests are completed. This callback does not indicate what collisions
     * occurred: each PhysicsBody's individual {@link PhysicsBody.CollisionListener}
     * will receive those collision callbacks.
     */
    public interface HitTestListener {

        /**
         * Invoked when a hit test has completed. Each individual {@link PhysicsBody} receives in
         * its {@link PhysicsBody.CollisionListener} a {@link
         * PhysicsBody.CollisionListener#onCollided(String, Vector, Vector)}
         * upon collision. This callback only returns if there was any hit.
         *
         * @param hasHit True if the hit test intersected with anything.
         */
        void onComplete(boolean hasHit);
    }

    private WeakReference<Scene> mScene;

    /**
     * @hide
     * @param scene
     */
    PhysicsWorld(Scene scene) {
        mScene = new WeakReference<Scene>(scene);
    }

    /**
     * Set the constant gravity force to use in the physics simulation. Gravity globally accelerates
     * physics bodies in a specific direction. This defaults to Earth's gravitational force: {0,
     * -9.81, 0}. Not all physics bodies need to respond to gravity: this can be set via {@link
     * PhysicsBody#setUseGravity(boolean)}.
     *
     * @param gravity The gravity vector to use.
     */
    public void setGravity(Vector gravity) {
        Scene scene = mScene.get();
        if (scene != null) {
            scene.setPhysicsWorldGravity(gravity.toArray());
        }
    }

    /**
     * Enable or disable physics 'debug draw' mode. When this mode is enabled, the physics shapes
     * will be drawn around each object with a PhysicsBody. This is useful for understanding what
     * approximations the physics simulation is using during its computations.
     *
     * @param debugDraw True to enable debug draw mode.
     */
    public void setDebugDraw(boolean debugDraw) {
        Scene scene = mScene.get();
        if (scene != null) {
            scene.setPhysicsDebugDraw(debugDraw);
        }
    }

    /**
     * Find collisions between the ray from <tt>from</tt> to <tt>to</tt> with any {@link Node} objects
     * that have physics bodies.  Each individual {@link PhysicsBody} will receive in
     * its {@link PhysicsBody.CollisionListener} a {@link
     * PhysicsBody.CollisionListener#onCollided(String, Vector, Vector)}
     * event upon collision. The event will contain the tag given to this intersection test.
     *
     * @param from The source of the ray.
     * @param to The destination of the ray.
     * @param closest True to only collide with the closest object.
     * @param tag The tag to use to identify this ray test in the <tt>onCollision</tt> callback of each {@link PhysicsBody}.
     * @param callback The {@link HitTestListener} to use, which will be invoked when the test is completed.
     */
    public void findCollisionsWithRayAsync(Vector from, Vector to, boolean closest,
                                           String tag, HitTestListener callback) {
        Scene scene = mScene.get();
        if (scene != null) {
            scene.findCollisionsWithRayAsync(from.toArray(), to.toArray(), closest, tag, callback);
        }
    }

    /**
     * Find collisions between the {@link PhysicsShape} cast from <tt>from</tt> to <tt>to</tt> with
     * any {@link Node} objects that have physics bodies.  Each individual {@link PhysicsBody} will
     * receive in its {@link PhysicsBody.CollisionListener} a {@link
     * PhysicsBody.CollisionListener#onCollided(String, Vector, Vector)} event
     * upon collision. The event will contain the tag given to this intersection test.
     *
     * @param from     The source of the ray.
     * @param to       The destination of the ray.
     * @param shape    The shape to use for the collision test. Only {@link PhysicsShapeSphere} and
     *                 {@link PhysicsShapeBox} are supported.
     * @param tag      The tag to use to identify this ray test in the <tt>onCollision</tt> callback
     *                 of each {@link PhysicsBody}.
     * @param callback The {@link HitTestListener} to use, which will be invoked when the test is
     *                 completed.
     */
    public void findCollisionsWithShapeAsync(Vector from, Vector to, PhysicsShape shape,
                                             String tag, HitTestListener callback) {
        Scene scene = mScene.get();
        if (scene != null) {
            scene.findCollisionsWithShapeAsync(from.toArray(), to.toArray(), shape.getType(), shape.getParams(),
                    tag, callback);
        }
    }

}
