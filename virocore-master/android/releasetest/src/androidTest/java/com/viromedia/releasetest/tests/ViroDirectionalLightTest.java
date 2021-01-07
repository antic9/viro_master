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

package com.viromedia.releasetest.tests;

import android.graphics.Color;
import android.support.test.espresso.core.deps.guava.collect.Iterables;
import android.support.test.runner.AndroidJUnit4;

import com.viro.core.AmbientLight;
import com.viro.core.Box;
import com.viro.core.DirectionalLight;
import com.viro.core.Material;
import com.viro.core.Node;
import com.viro.core.Quad;
import com.viro.core.Sphere;
import com.viro.core.Surface;
import com.viro.core.Vector;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.Iterator;
import java.util.List;

/**
 * Created by manish on 10/25/17.
 */

@RunWith(AndroidJUnit4.class)
public class ViroDirectionalLightTest extends ViroBaseTest {
    private static final String TAG = ViroDirectionalLightTest.class.getName();
    private static final int INFLUENCE_BITMASK = 1;
    private DirectionalLight mDirectionalLight;

    @Override
    void configureTestScene() {
        final AmbientLight ambientLightJni = new AmbientLight(Color.WHITE, 100.0f);
        mScene.getRootNode().addLight(ambientLightJni);

        // Add directional light in -Z direction
        mDirectionalLight = new DirectionalLight();
        mDirectionalLight.setInfluenceBitMask(INFLUENCE_BITMASK);
        mDirectionalLight.setIntensity(1000);
        mDirectionalLight.setCastsShadow(true);
        mScene.getRootNode().addLight(mDirectionalLight);

        // Configure verticle surface
        final Node verticleSurfaceNode = new Node();
        final Quad surface = new Quad(5, 5);
        final float[] surfacePos = {0, 0, -5};
        verticleSurfaceNode.setPosition(new Vector(surfacePos));
        final Material surfaceMaterial = new Material();
        surfaceMaterial.setDiffuseColor(Color.WHITE);
        surfaceMaterial.setLightingModel(Material.LightingModel.BLINN);
        surface.setMaterials(Arrays.asList(surfaceMaterial));
        verticleSurfaceNode.setGeometry(surface);
        verticleSurfaceNode.setLightReceivingBitMask(INFLUENCE_BITMASK);
        mScene.getRootNode().addChildNode(verticleSurfaceNode);

        // Configure horizontal surface
        final Node horizontalSurfaceNode = new Node();
        final Quad hSurface = new Quad(5, 5);
        final float[] hSurfacePos = {0, -2.5f, -5};
        horizontalSurfaceNode.setPosition(new Vector(hSurfacePos));
        final Material hSurfaceMaterial = new Material();
        hSurfaceMaterial.setDiffuseColor(Color.WHITE);
        hSurfaceMaterial.setLightingModel(Material.LightingModel.BLINN);
        hSurface.setMaterials(Arrays.asList(hSurfaceMaterial));
        horizontalSurfaceNode.setGeometry(hSurface);
        horizontalSurfaceNode.setRotation(new Vector(Math.toRadians(-90), 0, 0));
        horizontalSurfaceNode.setLightReceivingBitMask(INFLUENCE_BITMASK);
        mScene.getRootNode().addChildNode(horizontalSurfaceNode);

        // Configure sphere
        final Node sphereNode = new Node();
        final Sphere sphere = new Sphere(0.5f);
        final Material sphereMaterial = new Material();
        sphereMaterial.setDiffuseColor(Color.WHITE);
        sphereMaterial.setLightingModel(Material.LightingModel.BLINN);
        sphereNode.setShadowCastingBitMask(INFLUENCE_BITMASK);
        sphereNode.setLightReceivingBitMask(INFLUENCE_BITMASK);
        sphereNode.setGeometry(sphere);

        final float[] spherePos = {0, 0, -2f};
        sphereNode.setPosition(new Vector(spherePos));
        sphere.setMaterials(Arrays.asList(sphereMaterial));

        mScene.getRootNode().addChildNode(sphereNode);




    }

    @Test
    public void testDirectionalLight() {

        runUITest(() -> testSetColor());
        runUITest(() -> testSetIntensity());
        runUITest(() -> testSetCastsShadow());
        runUITest(() -> testSetDirection());

        // TODO Fix tests - VIRO-2428.
//        testSetInfluenceBitMask();
//        testSetShadowOrthographicSize();
//        testSetShadowOrthographicPosition();
//        testSetShadowMapSize();
//        testSetShadowBias();
//        testSetShadowNearZ();
//        testSetShadowFarZ();
//        testSetShadowOpacity();
    }

    /**
     * Base Light.java props
     **/

    private void testSetColor() {
        final List<Integer> colors = Arrays.asList(Color.WHITE, Color.RED, Color.GREEN, Color.BLUE, Color.MAGENTA, Color.CYAN);

        final Iterator<Integer> itr = Iterables.cycle(colors).iterator();

        mMutableTestMethod = () -> {
            mDirectionalLight.setColor(itr.next());
        };
        assertPass("Cycling colors through WHITE, RED, GREEN, BLUE, MAGENTA, CYAN");

    }

    private void testSetIntensity() {
        mMutableTestMethod = () -> {
            if (mDirectionalLight != null) {
                final float currentIntensity = mDirectionalLight.getIntensity();
                mDirectionalLight.setIntensity((currentIntensity + 200) % 2000);
            }
        };
        assertPass("Cycling intensity from 0 to 2000, with +200 every second");
    }

    private void testSetCastsShadow() {
        mMutableTestMethod = () -> {
            if (mDirectionalLight != null) {
                mDirectionalLight.setCastsShadow(!mDirectionalLight.getCastsShadow());
            }
        };
        assertPass("Toggling casts shadow");
    }

    private void testSetInfluenceBitMask() {
        assertPass("running testColorChange()");

    }


    private void testSetDirection() {
        final List<Vector> directions = Arrays.asList(new Vector(0, 0, -1), new Vector(0, -1, -1), new Vector(0, -1, 0));
        final Iterator<Vector> itr = Iterables.cycle(directions).iterator();
        mMutableTestMethod = () -> {
            if (mDirectionalLight != null) {
                mDirectionalLight.setDirection(itr.next());
            }
        };

        assertPass("Changing direction of light from -z to -y axis", () -> {
            mDirectionalLight.setDirection(directions.get(0));
        });
    }


    private void testSetShadowOrthographicSize() {
        assertPass("running testColorChange()");

    }

    private void testSetShadowOrthographicPosition() {
        assertPass("running testColorChange()");

    }

    private void testSetShadowMapSize() {
        assertPass("running testColorChange()");

    }

    private void testSetShadowBias() {
        assertPass("running testColorChange()");

    }

    private void testSetShadowNearZ() {
        assertPass("running testColorChange()");

    }

    private void testSetShadowFarZ() {
        assertPass("running testColorChange()");

    }

    private void testSetShadowOpacity() {
        assertPass("running testColorChange()");

    }
}
