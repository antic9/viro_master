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

/**
 * Callback interface for responding to click events, which occur when any Controller button is
 * clicked.
 */
public interface ClickListener {


    /**
     * Callback when a click event is registered over the given {@link Node}.
     *
     * @param source     The platform specific source ID, which indicates what button or
     *                   component on the Controller triggered the event. See the Controller's
     *                   Guide for information.
     * @param node       The {@link Node} that was clicked.
     * @param location   The location of the event in world coordinates.
     */
    void onClick(int source, Node node, Vector location);

    /**
     * Callback when the {@link ClickState} is changed over the given {@link Node}. This callback is
     * for receiving fine-grained information about a click: when the pointer goes down, when it
     * goes up, and if/when the 'click' itself is registered. For a completed click, this callback
     * is invoked <i>three</i> times: on {@link ClickState#CLICK_DOWN}, {@link ClickState#CLICK_UP},
     * and then for {@link ClickState#CLICKED}.
     * <p>
     * To simply listen for click events, use {@link #onClick(int, Node, Vector)}.
     *
     * @param source     The platform specific source ID, which indicates what button or component
     *                   on the Controller triggered the event. See the Controller's Guide for
     *                   information.
     * @param node       The {@link Node} that was clicked.
     * @param clickState The status of the click event.
     * @param location   The location of the event in world coordinates.
     */
    void onClickState(int source, Node node, ClickState clickState, Vector location);
}
