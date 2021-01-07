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

import android.graphics.Bitmap;
import android.graphics.Color;
import android.util.Log;

import com.viro.core.AmbientLight;
import com.viro.core.Material;
import com.viro.core.Node;
import com.viro.core.Sphere;
import com.viro.core.Texture;
import com.viro.core.Vector;
import com.viro.core.ClickListener;
import com.viro.core.ClickState;

import org.junit.Test;

import java.util.Arrays;

/**
 * Created by vadvani on 10/29/17.
 */

public class ViroSphereTest extends ViroBaseTest {
    private Sphere mSphereOne;
    private Sphere mSphereTwo;

    public ViroSphereTest() {
        super();
        mSphereOne = null;
        mSphereTwo = null;
    }

    @Override
    void configureTestScene() {
        final AmbientLight ambientLightJni = new AmbientLight(Color.WHITE, 1000.0f);
        mScene.getRootNode().addLight(ambientLightJni);

        // Creation of first sphere using radius constructor.
        final Material material = new Material();
        final Bitmap bobaBitmap = getBitmapFromAssets(mActivity, "boba.png");
        final Texture bobaTexture = new Texture(bobaBitmap, Texture.Format.RGBA8, true, true);
        material.setDiffuseTexture(bobaTexture);
        material.setDiffuseColor(Color.RED);
        material.setLightingModel(Material.LightingModel.BLINN);

        final Node node = new Node();
        mSphereOne = new Sphere(5);
        node.setGeometry(mSphereOne);
        final float[] spherePosition = {-6.5f, 0, -15};
        node.setPosition(new Vector(spherePosition));
        mSphereOne.setMaterials(Arrays.asList(material));
        node.setClickListener(new ClickListener() {
            @Override
            public void onClick(final int source, final Node node, final Vector location) {
                Log.i("ViroSphereTest", "Click listener on sphere one invoked");
            }

            @Override
            public void onClickState(int source, Node node, ClickState clickState, Vector location) {

            }
        });
        mScene.getRootNode().addChildNode(node);

        //Create of second sphere using other constructor.
        final Material materialBlue = new Material();
        materialBlue.setDiffuseColor(Color.BLUE);
        materialBlue.setLightingModel(Material.LightingModel.BLINN);

        final Node nodeTwo = new Node();
        mSphereTwo = new Sphere(6, 10, 10, true);
        nodeTwo.setGeometry(mSphereTwo);
        final float[] sphereTwoPosition = {6.5f, 0, -15};
        nodeTwo.setPosition(new Vector(sphereTwoPosition));
        nodeTwo.setClickListener(new ClickListener() {
            @Override
            public void onClick(final int source, final Node node, final Vector location) {
                Log.i("ViroSphereTest", "Click listener on sphere two invoked");
            }

            @Override
            public void onClickState(int source, Node node, ClickState clickState, Vector location) {

            }
        });
        mSphereTwo.setMaterials(Arrays.asList(materialBlue));
        mScene.getRootNode().addChildNode(nodeTwo);
    }

    @Test
    public void sphereTest() {
        runUITest(() -> sphereHeightSegmentCount());
        runUITest(() -> sphereWidthSegmentCount());
        runUITest(() -> sphereRadiusCount());
        runUITest(() -> sphereFaceOutward());
    }


    public void sphereHeightSegmentCount() {
        mMutableTestMethod = () -> {
            if (mSphereTwo != null && mSphereTwo.getHeightSegmentCount() < 30) {
                mSphereTwo.setHeightSegmentCount(mSphereTwo.getHeightSegmentCount() + 1);
            }
        };
        Log.i("ViroSphereTest", "sphereHeightSegmentCount");
        assertPass("Blue sphere changed in height segment from 10 to 30");
    }

    public void sphereWidthSegmentCount() {
        mMutableTestMethod = () -> {
            if (mSphereTwo != null && mSphereTwo.getWidthSegmentCount() < 30) {
                mSphereTwo.setWidthSegmentCount(mSphereTwo.getWidthSegmentCount() + 1);
            }
        };
        Log.i("ViroSphereTest", "sphereWidthSegmentCount");
        assertPass("Blue sphere changed in width segment from 10 to 30");
    }

    public void sphereRadiusCount() {
        mMutableTestMethod = () -> {
            if (mSphereOne != null && mSphereOne.getRadius() < 5) {
                mSphereOne.setRadius(mSphereOne.getRadius() + 1);
            }
        };
        assertPass("Boba sphere changed in radius segment from 5 to 10");
    }

    public void sphereFaceOutward() {
        mSphereOne.setFacesOutward(false);
        assertPass("Boba sphere flipped face outward to false.");
    }
}
