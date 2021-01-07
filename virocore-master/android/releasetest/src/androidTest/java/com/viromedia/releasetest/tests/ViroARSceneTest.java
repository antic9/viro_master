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
import android.support.test.espresso.core.deps.guava.collect.Iterables;
import android.util.Log;

import com.viro.core.ARAnchor;
import com.viro.core.ARNode;
import com.viro.core.ARPointCloud;
import com.viro.core.ARScene;
import com.viro.core.AmbientLight;
import com.viro.core.Box;
import com.viro.core.Material;
import com.viro.core.Node;
import com.viro.core.PointCloudUpdateListener;
import com.viro.core.Quad;
import com.viro.core.Surface;
import com.viro.core.Text;
import com.viro.core.Texture;
import com.viro.core.Vector;

import org.junit.Test;

import java.util.Arrays;
import java.util.Iterator;
import java.util.List;

/**
 * Created by vadvani on 11/7/17.
 * This includes tests for all methods for ARScene except the onAnchor callbacks. Those are tested in ViroARNodeTest.
 */

public class ViroARSceneTest extends ViroBaseTest {

    private ARScene mARScene;
    private Node mBoxNode;
    private AmbientLight mAmbientLight;
    private Text mTestText;

    private boolean mAmbientLightUpdateTestStarted = false;
    @Override
    void configureTestScene() {
        mAmbientLight = new AmbientLight(Color.WHITE, 1000.0f);
        mScene.getRootNode().addLight(mAmbientLight);
        mARScene = (ARScene)mScene;

        final Bitmap bobaBitmap = getBitmapFromAssets(mActivity, "boba.png");
        final Texture bobaTexture = new Texture(bobaBitmap, Texture.Format.RGBA8, true, true);
        final Material material = new Material();
        material.setDiffuseTexture(bobaTexture);
        material.setLightingModel(Material.LightingModel.BLINN);
        Box box = new Box(1, 1, 1);
        box.setMaterials(Arrays.asList(material));
        mBoxNode = new Node();
        mBoxNode.setGeometry(box);
        mBoxNode.setPosition(new Vector(0, 0, -4));
        mScene.getRootNode().addChildNode(mBoxNode);

        mTestText = new Text(mViroView.getViroContext(),
                "No input yet.", "Roboto", 12,
                Color.WHITE, 1f, 1f, Text.HorizontalAlignment.LEFT,
                Text.VerticalAlignment.TOP, Text.LineBreakMode.WORD_WRAP, Text.ClipMode.CLIP_TO_BOUNDS, 0);

        Node textNode = new Node();
        textNode.setPosition(new Vector(0, -1f, -4f));
        textNode.setGeometry(mTestText);
        mScene.getRootNode().addChildNode(textNode);

        mARScene.setListener(new ARScene.Listener() {
            @Override
            public void onTrackingInitialized() {
                mTestText.setText("AR Initialized callback received!");
            }

            @Override
            public void onAmbientLightUpdate(float lightIntensity, Vector color) {
                if(mAmbientLightUpdateTestStarted) {
                    mTestText.setText("Ambient light intensity:" + lightIntensity + ", color: " + color);
                    mAmbientLight.setIntensity(lightIntensity);
                }
            }

            @Override
            public void onAnchorFound(ARAnchor anchor, ARNode node) {

            }

            @Override
            public void onAnchorUpdated(ARAnchor anchor, ARNode node) {

            }

            @Override
            public void onAnchorRemoved(ARAnchor anchor, ARNode node) {

            }

            @Override
            public void onTrackingUpdated(ARScene.TrackingState state, ARScene.TrackingStateReason reason) {

            }

        });
        mARScene.setPointCloudUpdateListener(null);
        mARScene.resetPointCloudQuad();
    }

    @Test
    public void testARScene() {
        runUITest(() -> testARInitialized());
        runUITest(() -> testARAmbientLightValues());
        runUITest(() -> testPointCloudUpdateCallback());
        runUITest(() -> testDisplayPointCloudOn());
        runUITest(() -> testPointCloudScale());
        runUITest(() -> testPointCloudQuad());
        runUITest(() -> setPointCloudMaxPoints());
        runUITest(() -> testDisplayPointCloudOff());
    }

    private void testARInitialized() {
        assertPass("AR init text appears saying callback received.");
    }

    private void testARAmbientLightValues() {
        mAmbientLightUpdateTestStarted = true;
        assertPass("AR text udpates to show new ambient light values, ambient light changes to reflect new changes.");
    }

    private void testPointCloudScale() {
        mARScene.displayPointCloud(true);
        final List<Float> scaleSizes = Arrays.asList(0.1f, 0.2f, 0.3f, 0.4f, 0.5f);
        final Iterator<Float> itr = Iterables.cycle(scaleSizes).iterator();

        mMutableTestMethod = () -> {
            Float scaleNum = itr.next();
            mARScene.setPointCloudQuadScale(new Vector(scaleNum, scaleNum, scaleNum));
        };
        assertPass("Point cloud scale should change over time.", () -> {

        });
    }

    private void testDisplayPointCloudOn() {
        mARScene.displayPointCloud(true);
        assertPass("Display point cloud ");
    }

    private void testPointCloudUpdateCallback() {
        mARScene.displayPointCloud(true);
        final Text pointCloudText = new Text(mViroView.getViroContext(),
                "Waiting for cloud updates.", "Roboto", 12,
                Color.WHITE, 4f, 4f, Text.HorizontalAlignment.LEFT,
                Text.VerticalAlignment.TOP, Text.LineBreakMode.WORD_WRAP, Text.ClipMode.CLIP_TO_BOUNDS, 0);

        final Node textNode = new Node();
        textNode.setPosition(new Vector(0, 1f, -4f));
        textNode.setGeometry(pointCloudText);
        mScene.getRootNode().addChildNode(textNode);
        mARScene.setPointCloudUpdateListener(new PointCloudUpdateListener() {
            @Override
            public void onUpdate(ARPointCloud pointCloud) {
                Log.i("ViroARSceneTest", "Point cloud values: " + pointCloud.size());
                float []pointCloudArray = pointCloud.getPoints();
                long []ids =  pointCloud.getIds();
                String pointCloudStr = "Point clouds: ";
                for(int i =0; i< pointCloud.size(); i++) {
                    float x= pointCloudArray[i*4+0];
                    float y = pointCloudArray[i*4 + 1];
                    float z = pointCloudArray[i*4 + 2];
                    long pointID = ids[i];
                    pointCloudStr += "(" + x + "," + y + "," + z  + "), ";
                    Log.i("ViroARSceneTest", "point i" + i + "(x,y,z) id ->" + "(" + x + "," + y + "," + z  + ") id:" + pointID);
                }

                pointCloudText.setText(pointCloudStr);
            }
        });

        assertPass("Point cloud callback should update with new values");
    }

    private void testPointCloudQuad() {
        mARScene.displayPointCloud(true);
        Quad quadOne = new Quad(.3f, .3f);
        Quad quadTwo = new Quad(.3f, .3f);

        Material material = new Material();
        material.setLightingModel(Material.LightingModel.BLINN);
        material.setDiffuseColor(Color.BLUE);
        quadOne.setMaterials(Arrays.asList(material));

        Material materialTwo = new Material();
        materialTwo.setLightingModel(Material.LightingModel.BLINN);
        materialTwo.setDiffuseColor(Color.RED);
        quadTwo.setMaterials(Arrays.asList(materialTwo));

        final List<Quad> surfaces = Arrays.asList(quadOne, quadTwo);
        final Iterator<Quad> itr = Iterables.cycle(surfaces).iterator();
        mMutableTestMethod = ()->{
            Quad quad = itr.next();
            mARScene.setPointCloudQuad(quad);
        };
        assertPass("Point cloud surfaces should loop change from red to blue.", () -> {
        });
    }

    private void setPointCloudMaxPoints() {
        mARScene.displayPointCloud(true);
        Quad quadOne = new Quad(.5f, .5f);

        Material material = new Material();
        material.setLightingModel(Material.LightingModel.BLINN);
        material.setDiffuseColor(Color.BLUE);
        quadOne.setMaterials(Arrays.asList(material));
        mARScene.setPointCloudQuad(quadOne);
        mARScene.setPointCloudMaxPoints(2);
        final List<Integer> maxPoints = Arrays.asList(50, 200);
        final Iterator<Integer> itr = Iterables.cycle(maxPoints).iterator();

        mMutableTestMethod = ()->{
            mARScene.setPointCloudMaxPoints(itr.next());
        };

        assertPass("Max cloud points loops from 50 to 200", () -> {
        });
    }

    private void testDisplayPointCloudOff() {
        mARScene.displayPointCloud(true);
        mMutableTestMethod = ()->{
            mARScene.displayPointCloud(false);
        };
        assertPass("Display point cloud turns OFF");
    }
}
