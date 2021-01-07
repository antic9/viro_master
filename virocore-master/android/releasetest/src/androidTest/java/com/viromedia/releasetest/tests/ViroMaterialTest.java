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

import com.viro.core.AmbientLight;
import com.viro.core.Box;
import com.viro.core.Camera;
import com.viro.core.DirectionalLight;
import com.viro.core.Material;
import com.viro.core.Node;
import com.viro.core.Quad;
import com.viro.core.Sphere;

import com.viro.core.Spotlight;
import com.viro.core.Texture;
import com.viro.core.Vector;

import org.junit.Test;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.EnumSet;
import java.util.Iterator;
import java.util.List;


public class ViroMaterialTest extends ViroBaseTest {

    private Material mMaterial;
    private Sphere mSphere;
    private Node mSphereNode;
    private Box mBox;
    private Node mNodeBox;
    private AmbientLight mAmbientLight;
    private DirectionalLight mDirectionalLight;
    private ArrayList<Material> mMaterialBoxList;
    private Texture mBobaTexture;
    private Texture mSpecularTexture;

    @Override
    void configureTestScene() {
        mScene.setBackgroundCubeWithColor(Color.BLUE);
        mAmbientLight = new AmbientLight(Color.WHITE, 200);
        mScene.getRootNode().addLight(mAmbientLight);
        mDirectionalLight = new DirectionalLight(Color.RED, 1000.0f, new Vector(0, -1, -1));
        mScene.getRootNode().addLight(mDirectionalLight);

        Bitmap bobaBitmap = this.getBitmapFromAssets(mActivity, "boba.png");
        mBobaTexture = new Texture(bobaBitmap, Texture.Format.RGBA8, true, true);

        Bitmap specularBitmap = this.getBitmapFromAssets(mActivity, "specular.png");
        mSpecularTexture = new Texture(specularBitmap, Texture.Format.RGBA8, true, true);

        mMaterial = new Material();
        mMaterial.setDiffuseTexture(mBobaTexture);
        mMaterial.setDiffuseColor(Color.RED);
        mMaterial.setSpecularTexture(mSpecularTexture);
        mMaterial.setLightingModel(Material.LightingModel.PHONG);

        mSphere = new Sphere(5);
        mSphere.setMaterials(Arrays.asList(mMaterial));

        mSphereNode = new Node();
        mSphereNode.setGeometry(mSphere);
        mSphereNode.setPosition(new Vector(0, 0, -12));
        mScene.getRootNode().addChildNode(mSphereNode);

        mBox = new Box(2,2,2);
        mNodeBox = new Node();
        mNodeBox.setOpacity(0.8f);
        mNodeBox.setPosition(new Vector(0, 0, -4));
        mNodeBox.setRotation(new Vector(0f, 45f, 0f));
        mMaterialBoxList = new ArrayList();
        int[] colorArray = {Color.DKGRAY, Color.GRAY, Color.LTGRAY, Color.MAGENTA, Color.CYAN, Color.YELLOW};
        for (int i = 0; i < 6; i++) {
            Material material = new Material();
            material.setLightingModel(Material.LightingModel.CONSTANT);
            material.setDiffuseColor(colorArray[i]);
            mMaterialBoxList.add(material);
        }
        mBox.setMaterials(mMaterialBoxList);
        mNodeBox.setGeometry(mBox);

        Camera camera = new Camera();
        camera.setFieldOfView(90);
        mScene.getRootNode().setCamera(camera);
        mViroView.setPointOfView(mScene.getRootNode());
    }

    @Test
    public void testMaterial() {
        runUITest(() -> testMaterialLightingModelConstant());
        runUITest(() -> testMaterialLightingModelLambert());
        runUITest(() -> testMaterialLightingModelBlinn());
        runUITest(() -> testMaterialLightingModelPhong());

        runUITest(() -> testMaterialNormalMapTexture());
        runUITest(() -> testMaterialShininess());

        runUITest(() -> testMaterialBlendNone());
        runUITest(() -> testMaterialBlendAlpha());
        runUITest(() -> testMaterialBlendAdd());
        runUITest(() -> testMaterialBlendSubtract());
        runUITest(() -> testMaterialBlendMultiply());
        runUITest(() -> testMaterialBlendScreen());

        runUITest(() -> testMaterialTransparencyModeAOne());
        runUITest(() -> testMaterialTransparencyModeRGBZero());

        runUITest(() -> testMaterialCullMode());
        runUITest(() -> testMaterialReadsFromDepthBuffer());
        runUITest(() -> testMaterialWritesToDepthBuffer());
        runUITest(() -> testMaterialBloomThreshold());
        
        runUITest(() -> testMaterialColorWriteMaskCycling());
        runUITest(() -> testMaterialColorWriteMaskMultipleCycling());
        runUITest(() -> testMaterialColorWriteMaskOcclusion());
    }

    private void testMaterialLightingModelConstant() {
        mMaterial.setDiffuseColor(Color.WHITE);
        mMaterial.setLightingModel(Material.LightingModel.CONSTANT);
        assertPass("Lighting model on material is Constant");
    }

    private void testMaterialLightingModelLambert() {
        mMaterial.setDiffuseColor(Color.WHITE);
        mMaterial.setLightingModel(Material.LightingModel.LAMBERT);
        assertPass("Lighting model on material is Lambert");
    }

    private void testMaterialLightingModelBlinn() {
        mMaterial.setDiffuseColor(Color.WHITE);
        mMaterial.setLightingModel(Material.LightingModel.BLINN);
        mSphere.setMaterials(Arrays.asList(mMaterial));
        assertPass("Lighting model on material is Blinn");
    }

    private void testMaterialLightingModelPhong() {
        mMaterial.setDiffuseColor(Color.WHITE);
        mMaterial.setLightingModel(Material.LightingModel.PHONG);
        assertPass("Lighting model on material is Phong");
    }

    private void testMaterialNormalMapTexture() {
        mMaterial.setDiffuseColor(Color.WHITE);

        Bitmap specBitmap = this.getBitmapFromAssets(mActivity, "earth_normal.jpg");
        Texture specTexture = new Texture(specBitmap, Texture.Format.RGBA8, true, true);
        mMaterial.setNormalMap(specTexture);
        mSphere.setMaterials(Arrays.asList(mMaterial));
        assertPass("Added normal map to the material.");
    }

    private void testMaterialShininess() {
        mMaterial.setDiffuseColor(Color.WHITE);

        mMutableTestMethod = () -> {
            if (mMaterial != null && mMaterial.getShininess() < 10) {
                mMaterial.setShininess(mMaterial.getShininess() + .5f);
                mSphere.setMaterials(Arrays.asList(mMaterial));
            }
        };
        assertPass("Shininess increases over time.");
    }

    private void testMaterialBlendNone() {
        mScene.setBackgroundCubeWithColor(Color.WHITE);
        mSphereNode.setOpacity(0.5f);
        mMaterial.setDiffuseTexture(null);
        mMaterial.setSpecularTexture(null);
        mMaterial.setLightingModel(Material.LightingModel.CONSTANT);
        mMaterial.setBlendMode(Material.BlendMode.NONE);
        assertPass("Blend mode is NONE: red sphere over white background");
    }

    private void testMaterialBlendAlpha() {
        mScene.setBackgroundCubeWithColor(Color.WHITE);
        mSphereNode.setOpacity(0.5f);
        mMaterial.setDiffuseTexture(null);
        mMaterial.setSpecularTexture(null);
        mMaterial.setLightingModel(Material.LightingModel.CONSTANT);
        mMaterial.setBlendMode(Material.BlendMode.ALPHA);
        assertPass("Blend mode is Alpha: faded red sphere over white background");
    }

    private void testMaterialBlendAdd() {
        mScene.setBackgroundCubeWithColor(Color.GREEN);
        mSphereNode.setOpacity(0.5f);
        mMaterial.setDiffuseTexture(null);
        mMaterial.setSpecularTexture(null);
        mMaterial.setLightingModel(Material.LightingModel.CONSTANT);
        mMaterial.setBlendMode(Material.BlendMode.ADD);
        assertPass("Blend mode is ADD: yellow sphere over green background");
    }

    private void testMaterialBlendSubtract() {
        mScene.setBackgroundCubeWithColor(Color.YELLOW);
        mSphereNode.setOpacity(0.5f);
        mMaterial.setDiffuseTexture(null);
        mMaterial.setSpecularTexture(null);
        mMaterial.setLightingModel(Material.LightingModel.CONSTANT);
        mMaterial.setBlendMode(Material.BlendMode.SUBTRACT);
        assertPass("Blend mode is SUBTRACT: green sphere over yellow background");
    }

    private void testMaterialBlendMultiply() {
        mScene.setBackgroundCubeWithColor(Color.GRAY);
        mMaterial.setDiffuseTexture(null);
        mMaterial.setDiffuseColor(Color.GRAY);
        mMaterial.setSpecularTexture(null);
        mMaterial.setLightingModel(Material.LightingModel.CONSTANT);
        mMaterial.setBlendMode(Material.BlendMode.MULTIPLY);
        assertPass("Blend mode is MULTIPLY: sphere is darker gray than background");
    }

    private void testMaterialBlendScreen() {
        mScene.setBackgroundCubeWithColor(Color.GRAY);
        mMaterial.setDiffuseTexture(null);
        mMaterial.setDiffuseColor(Color.GRAY);
        mMaterial.setSpecularTexture(null);
        mMaterial.setLightingModel(Material.LightingModel.CONSTANT);
        mMaterial.setBlendMode(Material.BlendMode.SCREEN);
        assertPass("Blend mode is SCREEN: sphere is lighter gray than background");
    }

    private void testMaterialTransparencyModeAOne() {
        mScene.setBackgroundCubeWithColor(Color.BLUE);
        mMaterial.setDiffuseColor(Color.RED);
        mMaterial.setDiffuseTexture(mBobaTexture);
        mMaterial.setTransparencyMode(Material.TransparencyMode.A_ONE);
        mSphere.setMaterials(Arrays.asList(mMaterial));
        assertPass("TransparencyMode set to A_ONE");
    }

    private void testMaterialTransparencyModeRGBZero() {
        mScene.setBackgroundCubeWithColor(Color.BLUE);
        mMaterial.setDiffuseColor(Color.RED);
        mMaterial.setDiffuseTexture(mBobaTexture);
        mMaterial.setTransparencyMode(Material.TransparencyMode.RGB_ZERO);
        mSphere.setMaterials(Arrays.asList(mMaterial));
        assertPass("TransparencyMode set to RGB_ZERO");
    }

    private void testMaterialCullMode() {
        mScene.getRootNode().addChildNode(mNodeBox);
        mMutableTestMethod = () -> {
            Material.CullMode newCullMode = Material.CullMode.BACK;
            if (mMaterialBoxList.get(0).getCullMode() == Material.CullMode.BACK) {
                 newCullMode  = Material.CullMode.FRONT;
            } else if(mMaterialBoxList.get(0).getCullMode() == Material.CullMode.FRONT) {
                newCullMode = Material.CullMode.NONE;
            } else if(mMaterialBoxList.get(0).getCullMode() == Material.CullMode.NONE) {
                newCullMode = Material.CullMode.BACK;
            }

            for(Material material: mMaterialBoxList) {
                material.setCullMode(newCullMode);
            }
            mBox.setMaterials(mMaterialBoxList);
        };
        assertPass("CullMode loops from FRONT to BACK to NONE.");
    }

    private void testMaterialReadsFromDepthBuffer() {
        mScene.getRootNode().addChildNode(mNodeBox);
        mNodeBox.setOpacity(1.0f);

        mMutableTestMethod = () -> {
            mMaterial.setReadsFromDepthBuffer(!mMaterial.getReadsFromDepthBuffer());
        };
        assertPass("Sphere should flip from being drawn in front of box to behind.");
    }

    private void testMaterialWritesToDepthBuffer() {
        mScene.getRootNode().addChildNode(mNodeBox);
        mNodeBox.setOpacity(1.0f);
        mNodeBox.setPosition(new Vector(0, 0, -15));
        mNodeBox.setRenderingOrder(1);

        mMutableTestMethod = () -> {
            mMaterial.setWritesToDepthBuffer(!mMaterial.getWritesToDepthBuffer());
        };
        assertPass("Box should appear and disappear.");
    }

    private void testMaterialBloomThreshold() {
        mNodeBox.setPosition(new Vector(0, 0, -15));
        mMaterial.setBloomThreshold(0.0f);
        mMutableTestMethod = () -> {
            if (mMaterial.getBloomThreshold() <= 1.0f) {
                mMaterial.setBloomThreshold(mMaterial.getBloomThreshold() + .02f);
            }
        };
        assertPass("Bloom threshold changes low to high (sphere goes from bright to dim)");
    }

    private void testMaterialColorWriteMaskCycling() {
        mScene.getRootNode().removeLight(mDirectionalLight);
        mScene.setBackgroundCubeWithColor(Color.BLACK);
        mNodeBox.setPosition(new Vector(0, 0, -15));
        mMaterial.setDiffuseTexture(null);
        mMaterial.setDiffuseColor(Color.WHITE);

        final List<Material.ColorWriteMask> masks = Arrays.asList(Material.ColorWriteMask.ALL, Material.ColorWriteMask.RED,
                Material.ColorWriteMask.GREEN, Material.ColorWriteMask.BLUE,
                Material.ColorWriteMask.NONE);

        final Iterator<Material.ColorWriteMask> itr = Iterables.cycle(masks).iterator();
        mMutableTestMethod = () -> {
            mMaterial.setColorWriteMask(EnumSet.of(itr.next()));
        };
        assertPass("Color should cycle (WHITE -> RED -> GREEN -> BLUE -> BLACK)");
    }

    private void testMaterialColorWriteMaskMultipleCycling() {
        mScene.getRootNode().removeLight(mDirectionalLight);
        mScene.setBackgroundCubeWithColor(Color.BLACK);
        mNodeBox.setPosition(new Vector(0, 0, -15));
        mMaterial.setColorWriteMask(EnumSet.of(Material.ColorWriteMask.RED, Material.ColorWriteMask.GREEN));
        mMaterial.setDiffuseTexture(null);
        mMaterial.setDiffuseColor(Color.WHITE);

        final List<EnumSet<Material.ColorWriteMask>> masks = Arrays.asList(
                EnumSet.of(Material.ColorWriteMask.RED, Material.ColorWriteMask.GREEN),
                EnumSet.of(Material.ColorWriteMask.RED, Material.ColorWriteMask.BLUE),
                EnumSet.of(Material.ColorWriteMask.GREEN, Material.ColorWriteMask.BLUE)
        );

        final Iterator<EnumSet<Material.ColorWriteMask>> itr = Iterables.cycle(masks).iterator();
        mMutableTestMethod = () -> {
            mMaterial.setColorWriteMask(itr.next());
        };
        assertPass("Color should cycle (YELLOW, PURPLE, CYAN)");
    }

    private void testMaterialColorWriteMaskOcclusion() {
        Quad quad = new Quad(5, 5);

        Material material = new Material();
        material.setColorWriteMask(EnumSet.of(Material.ColorWriteMask.NONE));
        quad.setMaterials(Arrays.asList(material));

        final Node quadNode = new Node();
        quadNode.setGeometry(quad);
        quadNode.setRenderingOrder(-1);
        quadNode.setPosition(new Vector(0, 0, -4));

        mScene.getRootNode().addChildNode(quadNode);
        mMutableTestMethod = () -> {
            quadNode.setVisible(!quadNode.isVisible());
        };
        assertPass("The sphere should disappear (behind a transparent quad) then reappear. Background should stay blue.");
    }
}
