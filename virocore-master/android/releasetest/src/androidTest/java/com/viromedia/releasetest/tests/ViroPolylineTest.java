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
import android.os.Handler;
import android.support.test.espresso.core.deps.guava.collect.Iterables;

import com.viro.core.AmbientLight;
import com.viro.core.Material;
import com.viro.core.Node;
import com.viro.core.Polyline;
import com.viro.core.Vector;

import org.junit.Test;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;

/**
 * Created by manish on 11/6/17.
 */

public class ViroPolylineTest extends ViroBaseTest {
    private Node polylineNode;
    private Polyline polyline;
    private Handler mHandler;

    @Override
    void configureTestScene() {
        final AmbientLight ambientLight = new AmbientLight(Color.WHITE, 200);
        mScene.getRootNode().addLight(ambientLight);

        final Material material = new Material();
        material.setDiffuseColor(Color.RED);
        material.setLightingModel(Material.LightingModel.CONSTANT);
        material.setCullMode(Material.CullMode.NONE);
        final float[][] points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
        ArrayList<Vector> pointsList = new ArrayList<Vector>();
        pointsList.add(new Vector(0,0,0));
        pointsList.add(new Vector(1,0,0));
        pointsList.add(new Vector(1,1,0));

        polyline = new Polyline(pointsList, 0.1f);
        polyline.setMaterials(Arrays.asList(material));
        polylineNode = new Node();
        polylineNode.setGeometry(polyline);
        polylineNode.setPosition(new Vector(0, -1f, -3.3f));
        mScene.getRootNode().addChildNode(polylineNode);
    }

    @Test
    public void polylineTest() {
        runUITest(() -> testAppendPoints());
        runUITest(() -> testSetPoints());
        runUITest(() -> testSetThickness());
    }

    private void testAppendPoints() {
        final List<Vector> points = Arrays.asList(new Vector(0, 1, 0), new Vector(-1, 1, 0),
                new Vector(-1, 0, 0), new Vector(-1, -1, 0),
                new Vector(0, -1, 0), new Vector(1, -1, 0), new Vector(2, -1, 0),
                new Vector(2, 0, 0), new Vector(2, 1, 0), new Vector(2, 2, 0));

        final Iterator<Vector> itr = Iterables.cycle(points).iterator();

        mMutableTestMethod = () -> {
            polyline.appendPoint(itr.next());
        };

        assertPass("Appending new points every second");
    }

    private void testSetPoints() {

        final List<Vector> points1 = Arrays.asList(new Vector(0, 1, 0), new Vector(-1, 1, 0),
                new Vector(-1, 0, 0), new Vector(-1, -1, 0), new Vector(0, -1, 0));

        final List<Vector> points2 = Arrays.asList(new Vector(1, -1, 0), new Vector(2, -1, 0),
                new Vector(2, 0, 0), new Vector(2, 1, 0), new Vector(2, 2, 0));

        final List<List> pointSets = Arrays.asList(points1, points2);
        final Iterator<List> itr = Iterables.cycle(pointSets).iterator();
        mMutableTestMethod = () -> {

            polyline.setPoints(itr.next());
        };

        assertPass("Alternating between new points every second");
    }

    private void testSetThickness() {
        mMutableTestMethod = () -> {
            polyline.setThickness(polyline.getThickness() + 0.1f);
        };

        assertPass("Increasing thickness every second", () -> {
            polyline.setThickness(0.1f);
        });
    }
}
