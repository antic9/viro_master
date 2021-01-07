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
import android.graphics.Point;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

import com.viro.core.ARHitTestListener;
import com.viro.core.ARHitTestResult;
import com.viro.core.AmbientLight;
import com.viro.core.Material;
import com.viro.core.Node;
import com.viro.core.Sphere;
import com.viro.core.Vector;
import com.viro.core.ViroViewARCore;

import org.junit.Test;

import java.util.Arrays;

/**
 * Created by vadvani on 11/8/17.
 */

public class ViroARHitTest extends ViroBaseTest {

    private ViroViewARCore mViewARCore;

    @Override
    void configureTestScene() {
        AmbientLight light = new AmbientLight(Color.WHITE, 1000.0f);
        mScene.getRootNode().addLight(light);
        mViewARCore = (ViroViewARCore)mViroView;
    }

    @Test
    public void testARHitTest() {
        runUITest(() -> testARCameraHitTest());
        runUITest(() -> testPerformARHitTestWithRay());
        runUITest(() -> testPerformARHitTestWithPosition());
        runUITest(() -> testPerformARHitTest());
    }

    private void testPerformARHitTestWithRay() {
        mMutableTestMethod = () -> {
            Vector forward = mViroView.getLastCameraForwardRealtime();
            Vector fowardWithDistance = new Vector(forward.x * 2.f, forward.y * 2.f, forward.z * 2.f);

            mViewARCore.performARHitTestWithRay(fowardWithDistance, new ARHitTestListener() {
                public void onHitTestFinished(ARHitTestResult[] results) {
                    for (ARHitTestResult result : results) {
                        Sphere sphere = new Sphere(.1f);
                        Material material = new Material();
                        material.setLightingModel(Material.LightingModel.BLINN);
                        material.setDiffuseColor(Color.GREEN);
                        sphere.setMaterials(Arrays.asList(material));
                        Node nodeSphere = new Node();
                        nodeSphere.setPosition(result.getPosition());
                        nodeSphere.setGeometry(sphere);
                        mScene.getRootNode().addChildNode(nodeSphere);
                    }
                }
            });
        };

        assertPass("Should see periodic rendered results from AR hit tests using the forward ray (green balls)");
    }

    private void testARCameraHitTest() {
        mViewARCore.setCameraARHitTestListener(new ARHitTestListener() {
            @Override
            public void onHitTestFinished(ARHitTestResult[] results) {
                for(ARHitTestResult result: results) {
                    Sphere sphere = new Sphere(.1f);
                    Material material = new Material();
                    material.setLightingModel(Material.LightingModel.BLINN);
                    material.setDiffuseColor(Color.RED);
                    sphere.setMaterials(Arrays.asList(material));
                    Node nodeSphere = new Node();
                    nodeSphere.setPosition(result.getPosition());
                    nodeSphere.setGeometry(sphere);
                    mScene.getRootNode().addChildNode(nodeSphere);
                }
            }
        });

        assertPass("Should see *continuous* rendered results from Camera AR HIT Test in red", ()->{
            mViewARCore.setCameraARHitTestListener(null);
        });

    }

    private void testPerformARHitTestWithPosition() {
        mViewARCore.performARHitTestWithPosition(new Vector(0, 0, -3), new ARHitTestListener() {
            @Override
            public void onHitTestFinished(ARHitTestResult[] results) {
                for(ARHitTestResult result: results) {
                    Sphere sphere = new Sphere(.1f);
                    Material material = new Material();
                    material.setLightingModel(Material.LightingModel.BLINN);
                    material.setDiffuseColor(Color.BLUE);
                    sphere.setMaterials(Arrays.asList(material));
                    Node nodeSphere = new Node();
                    nodeSphere.setPosition(result.getPosition());
                    nodeSphere.setGeometry(sphere);
                    mScene.getRootNode().addChildNode(nodeSphere);
                }
            }
        });

        assertPass("Should see a SINGLE rendered result (green ball) from an AR hit test fired toward (0, 0, -3)", ()->{

        });
    }

    private void testPerformARHitTest() {
        mViewARCore.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if(event.getAction() == MotionEvent.ACTION_DOWN) {
                    event.getX();
                    event.getY();
                    mViewARCore.performARHitTest(new Point((int)event.getX(), (int)event.getY()), new ARHitTestListener() {
                        @Override
                        public void onHitTestFinished(ARHitTestResult[] results) {
                            for(ARHitTestResult result: results) {
                                Sphere sphere = new Sphere(.1f);
                                Material material = new Material();
                                material.setLightingModel(Material.LightingModel.BLINN);
                                material.setDiffuseColor(Color.BLUE);
                                sphere.setMaterials(Arrays.asList(material));
                                Node nodeSphere = new Node();
                                nodeSphere.setPosition(result.getPosition());
                                nodeSphere.setGeometry(sphere);
                                mScene.getRootNode().addChildNode(nodeSphere);
                            }
                        }
                    });
                }
                return false;
            }
        });

        assertPass("Tap to see a blue ball appear from firing a ray toward tapped location");

    }

}
