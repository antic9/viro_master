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
import android.net.Uri;

import com.viro.core.Material;
import com.viro.core.Node;
import com.viro.core.ParticleEmitter;
import com.viro.core.Quad;
import com.viro.core.Surface;
import com.viro.core.Texture;
import com.viro.core.Vector;
import com.viro.core.ViroContext;
import org.junit.Test;

import java.util.Arrays;

public class ViroParticleEmitterTest extends ViroBaseTest {
    private ParticleEmitter mEmitter;
    private Node mEmitterNode;

    public ViroParticleEmitterTest() {
        super();
    }

    private ParticleEmitter createBaseEmitter(){
        final ViroContext context = mViroView.getViroContext();
        float width = 2;
        float height = 2;

        // Construct particle Geometry.
        Quad quad = new Quad(1,1);
        quad.setWidth(width);
        quad.setHeight(height);

        // Construct Particle Image
        final Bitmap result = this.getBitmapFromAssets(mActivity, "particle_snow.png");
        final Texture snowTexture = new Texture(result, Texture.Format.RGBA8, true, true);
        Material material = new Material();
        material.setDiffuseTexture(snowTexture);
        quad.setMaterials(Arrays.asList(material));

        // Finally construct the particle with the geometry and node.
        return new ParticleEmitter(context, quad);
    }

    @Override
    void configureTestScene() {
        if (mEmitter != null) {
            mEmitter.dispose();
        }
        mEmitterNode = new Node();
        mEmitter = createBaseEmitter();
        mEmitterNode.setPosition(new Vector(0, -10, -15));
        mEmitterNode.setParticleEmitter(mEmitter);
        mScene.getRootNode().addChildNode(mEmitterNode);

        basicEmitterSetup();
    }

    /**
     * Detaches particle node from the scene and destroys it.
     */
    private void cleanupTest() {
        // Remove particle node from scene.
        mEmitterNode.removeFromParentNode();
        mEmitterNode.setParticleEmitter(null);
        mEmitter.dispose();
        mEmitterNode.dispose();
    }

    private void basicEmitterSetup() {
        mEmitter.setParticleLifetime(2000, 2000);
        mEmitter.setDuration(2000);
        mEmitter.setLoop(true);
        mEmitter.setDelay(0);
        mEmitter.setMaxParticles(200);
        mEmitter.setEmissionRatePerSecond(10, 10);
    }

    @Test
    public void particleEmitterTests() {
        runUITest(() -> blendModeTest(Material.BlendMode.ALPHA));
        runUITest(() -> blendModeTest(Material.BlendMode.ADD));
        runUITest(() -> blendModeTest(Material.BlendMode.SCREEN));
        runUITest(() -> singleParticleTest());
        runUITest(() -> emitterColorTest());
        runUITest(() -> emitterScaleTest());
        runUITest(() -> emitterRotationTest());
        runUITest(() -> emitterOpacityTest());
        runUITest(() -> randomizedPropertiesTest());
        runUITest(() -> multipleIntervalTest());
        runUITest(() -> emitterDistanceTest());
    }

    public void singleParticleTest() {
        mEmitter.setEmissionRatePerSecond(1, 1);
        mEmitter.run();
        assertPass("You should see 1 particle emit per second.");
    }

    public void emitterColorTest() {
        int colorStart = Color.RED;
        int colorEnd = Color.BLUE;

        ParticleEmitter.ParticleModifierColor mod
                = new ParticleEmitter.ParticleModifierColor(colorStart, colorStart);
        mod.addInterval(1000, colorEnd);
        mEmitter.setColorModifier(mod);
        mEmitter.run();

        assertPass("You should see the particle emit with red color and change to blue.");
    }

    public void emitterScaleTest() {
        Vector startSize = new Vector(1,1,0);
        Vector endSize = new Vector(3,3,0);

        ParticleEmitter.ParticleModifierVector mod
                = new ParticleEmitter.ParticleModifierVector(startSize, startSize);
        mod.addInterval(1000, endSize);
        mEmitter.setScaleModifier(mod);
        mEmitter.run();

        assertPass("Particles should scale.");
    }

    public void emitterRotationTest() {
        ParticleEmitter.ParticleModifierFloat mod
                = new ParticleEmitter.ParticleModifierFloat(0, 0);
        mod.addInterval(1000, 3.14f);
        mEmitter.setRotationModifier(mod);
        mEmitter.run();

        assertPass("Particles should rotate.");
    }

    public void emitterOpacityTest() {
        float opacityStart = 1.0f;
        float opacityEnd = 0.0f;

        ParticleEmitter.ParticleModifierFloat mod
                = new ParticleEmitter.ParticleModifierFloat(opacityStart, opacityStart);
        mod.addInterval(1000, opacityEnd);
        mEmitter.setOpacityModifier(mod);
        mEmitter.run();

        assertPass("Particles should fade out.");
    }

    public void randomizedPropertiesTest(){
        float opacityStart = 1.0f;
        float opacityEnd = 0.0f;
        float startRot = 0f;
        float endRot = 3.14f;
        Vector startScale = new Vector(1,1,0);
        Vector endScale = new Vector(3,3,0);
        int colorStart = Color.RED;
        int colorEnd = Color.BLUE;

        ParticleEmitter.ParticleModifierFloat mod1
                = new ParticleEmitter.ParticleModifierFloat(startRot, endRot);
        mod1.addInterval(1000, endRot);
        mEmitter.setRotationModifier(mod1);

        ParticleEmitter.ParticleModifierVector mod2
                = new ParticleEmitter.ParticleModifierVector(startScale, endScale);
        mod2.addInterval(1000, endScale);
        mEmitter.setScaleModifier(mod2);

        ParticleEmitter.ParticleModifierColor mod3
                = new ParticleEmitter.ParticleModifierColor(colorStart, colorEnd);
        mod3.addInterval(1000, colorEnd);
        mEmitter.setColorModifier(mod3);

        ParticleEmitter.ParticleModifierFloat mod4
                = new ParticleEmitter.ParticleModifierFloat(opacityStart, opacityEnd);
        mod4.addInterval(1000, opacityEnd);
        mEmitter.setOpacityModifier(mod4);
        mEmitter.run();

        assertPass("Particles should change color, grow, fade out, rotate, non-uniformly.");
    }

    public void blendModeTest(Material.BlendMode blendMode) {
        if (mEmitter != null) {
            mEmitter.dispose();
        }
        if (mEmitterNode != null) {
            mEmitterNode.removeFromParentNode();
        }
        Texture environment = Texture.loadRadianceHDRTexture(Uri.parse("file:///android_asset/ibl_mans_outside.hdr"));
        mScene.setBackgroundTexture(environment);

        mEmitterNode = new Node();
        mEmitterNode.setPosition(new Vector(0, -0.5, -1));

        // Particle surface
        Quad quad = new Quad(1, 1);
        final Bitmap result = this.getBitmapFromAssets(mActivity, "darkSmoke.png");

        final Texture texture = new Texture(result, Texture.Format.RGBA8, true, true);
        Material material = new Material();
        material.setDiffuseTexture(texture);
        quad.setMaterials(Arrays.asList(material));

        // Core emitter settings
        mEmitter = new ParticleEmitter(mViroView.getViroContext(), quad);
        mEmitter.setParticleLifetime(6000, 7000);
        mEmitter.setEmissionRatePerSecond(12, 18);
        mEmitter.setMaxParticles(100);
        mEmitter.setDuration(1000);
        mEmitter.setLoop(true);
        mEmitter.setBlendMode(blendMode);

        // Spawn volume
        ParticleEmitter.SpawnVolume volume = new ParticleEmitter.SpawnVolumeSphere(0.1f);
        mEmitter.setSpawnVolume(volume, false);

        // Scale modifier
        ParticleEmitter.ParticleModifierVector scale = new ParticleEmitter.ParticleModifierVector(new Vector(0.3, 0.3, 0.3), new Vector(0.3, 0.3, 0.3));
        scale.setFactor(ParticleEmitter.Factor.TIME);
        scale.addInterval(1000, new Vector(1, 1, 1));
        mEmitter.setScaleModifier(scale);

        // Opacity modifier
        ParticleEmitter.ParticleModifierFloat opacity = new ParticleEmitter.ParticleModifierFloat(0, 0);
        opacity.setFactor(ParticleEmitter.Factor.TIME);
        opacity.addInterval(1000, 1.0f);
        opacity.addInterval(1000, 1.0f);
        opacity.addInterval(1000, 0.0f);
        mEmitter.setOpacityModifier(opacity);

        // Velocity modifier
        ParticleEmitter.ParticleModifierVector velocity = new ParticleEmitter.ParticleModifierVector(new Vector(0.05, 0.15, 0.01 ),
                new Vector(-0.05, 0.15, 0.01));
        mEmitter.setVelocityModifier(velocity);

        mEmitterNode.setParticleEmitter(mEmitter);
        mScene.getRootNode().addChildNode(mEmitterNode);
        mEmitter.run();

        if (blendMode == Material.BlendMode.ALPHA) {
            assertPass("Smoke should fade in, be dark and opaque, then fade out");
        }
        else if (blendMode == Material.BlendMode.ADD) {
            assertPass("Smoke should be white and should brighten the background");
        }
        else if (blendMode == Material.BlendMode.SCREEN) {
            assertPass("Smoke should be white and should diminish the background");
        }
    }

    public void multipleIntervalTest(){
        int colorStart = Color.RED;
        int colorMid = Color.YELLOW;
        int ColorMid2 = Color.GREEN;
        int colorEnd = Color.BLUE;

        mEmitter.setParticleLifetime(3000,3000);
        ParticleEmitter.ParticleModifierColor mod
                = new ParticleEmitter.ParticleModifierColor(colorStart, colorStart);
        mod.addInterval(400, colorMid);
        mod.addInterval(1300, ColorMid2);
        mod.addInterval(2000, colorEnd);
        mEmitter.setColorModifier(mod);
        mEmitter.run();

        assertPass("Particles should turn into yellow, green, blue color.");
    }

    public void emitterDistanceTest(){
        int colorStart = Color.RED;
        int colorEnd = Color.BLUE;
        Vector startSize = new Vector(1,1,0);
        Vector endSize = new Vector(0,0,0);

        ParticleEmitter.ParticleModifierVector modScale
                = new ParticleEmitter.ParticleModifierVector(startSize, startSize);
        modScale.setFactor(ParticleEmitter.Factor.DISTANCE);
        modScale.addInterval(5, endSize);
        mEmitter.setScaleModifier(modScale);

        ParticleEmitter.ParticleModifierColor mod
                = new ParticleEmitter.ParticleModifierColor(colorStart, colorStart);
        mod.setFactor(ParticleEmitter.Factor.DISTANCE);
        mod.addInterval(2, colorEnd);
        mEmitter.setColorModifier(mod);

        mEmitter.run();

        assertPass("Distance Test: Particles should turn blue and shrink the further away it is from the spawn point.");
    }

}
