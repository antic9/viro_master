//
//  VROSkeletonRenderer.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 3/19/19.
//  Copyright © 2019 Viro Media. All rights reserved.
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

#include "VROSkeletonRenderer.h"
#include "VROBodyTrackeriOS.h"

static const float kConfidenceThreshold = 0.15;
static const bool kDrawLabels = false;

static const int kRecognitionNumColors = 16;
static std::string kPointLabels[16] = {
    "top\t\t\t", //0
    "neck\t\t", //1
    "R shoulder\t", //2
    "R elbow\t\t", //3
    "R wrist\t\t", //4
    "L shoulder\t", //5
    "L elbow\t\t", //6
    "L wrist\t\t", //7
    "R hip\t\t", //8
    "R knee\t\t", //9
    "R ankle\t\t", //10
    "L hip\t\t", //11
    "L knee\t\t", //12
    "L ankle\t\t", //13
    "Thorax\t\t", //14
    "Pelvis\t\t", //15
};

static UIColor *kColors[16] = {
    [UIColor brownColor],
    [UIColor brownColor],
    [UIColor blueColor],
    [UIColor blueColor],
    [UIColor blueColor],
    [UIColor greenColor],
    [UIColor greenColor],
    [UIColor greenColor],
    [UIColor redColor],
    [UIColor redColor],
    [UIColor redColor],
    [UIColor yellowColor],
    [UIColor yellowColor],
    [UIColor yellowColor],
    [UIColor brownColor],
    [UIColor brownColor],
};

std::vector<std::pair<int, int>> kSkeleton = {{0, 1}, {1, 14}, {14, 15}, {5, 1}, {2, 1}, {5, 6}, {6, 7},
    {2, 3}, {3, 4}, {11, 15}, {11, 12}, {12, 13}, {8, 15}, {8, 9}, {9, 10}};

VROSkeletonRenderer::VROSkeletonRenderer(VROViewAR *view, std::shared_ptr<VROBodyTracker> tracker) {
    _view = view;
    _bodyTracker = tracker;
    _drawDelegate = [[VROBodyRecognitionDrawDelegate alloc] init];
    [_view setDebugDrawDelegate:_drawDelegate];
}

VROSkeletonRenderer::~VROSkeletonRenderer() {
    
}

void VROSkeletonRenderer::onBodyJointsFound(const VROPoseFrame &joints) {
    int viewWidth  = _view.frame.size.width;
    int viewHeight = _view.frame.size.height;
    
    std::vector<VROVector3f> labelPositions;
    std::vector<NSString *> labels;
    std::vector<VROBoundingBox> boxes;
    std::vector<UIColor *> colors;
    std::vector<float> confidences;
    
    for (int i = 0; i < kNumBodyJoints; i++) {
        const std::vector<VROInferredBodyJoint> &kv = joints[i];
        if (kv.size() > 0) {
            VROInferredBodyJoint joint = kv[0];
            VROBoundingBox bounds = joint.getBounds();
            
            float x = bounds.getX() * viewWidth;
            float y = bounds.getY() * viewHeight;
            float width = bounds.getSpanX() * viewWidth;
            float height = bounds.getSpanY() * viewHeight;
            
            int colorIndex = (int) joint.getType();
            colorIndex = MAX(0, MIN(colorIndex, kRecognitionNumColors - 1));
            UIColor *color = kColors[colorIndex];
            
            VROVector3f labelPosition = { (float) (x - width / 2.0), (float) (y + height / 2.0), 0 };
            VROBoundingBox transformedBox(x - width / 2.0, x + width / 2.0, y - height / 2.0, y + height / 2.0, 0, 0);
            
            NSString *className = [NSString stringWithUTF8String:kPointLabels[i].c_str()];
            NSString *classAndConf = [NSString stringWithFormat:@"%@ [%.03f]", className, joint.getConfidence()];
            
            boxes.push_back(transformedBox);
            labels.push_back(classAndConf);
            labelPositions.push_back({ labelPosition.x, labelPosition.y, 0 });
            colors.push_back(color);
            confidences.push_back(joint.getConfidence());
        }
        else {
            boxes.push_back({});
            labels.push_back(@"");
            labelPositions.push_back({0, 0, 0});
            colors.push_back(nil);
            confidences.push_back(0);
        }
    }
    
    std::shared_ptr<VROBodyTracker> tracker = _bodyTracker.lock();
    if (tracker) {
        std::shared_ptr<VROBodyTrackeriOS> trackeriOS = std::dynamic_pointer_cast<VROBodyTrackeriOS>(tracker);
        [_drawDelegate setDynamicCropBox:trackeriOS->getDynamicCropBox()];
    }
    [_drawDelegate setBoxes:boxes];
    [_drawDelegate setLabels:labels positions:labelPositions];
    [_drawDelegate setColors:colors];
    [_drawDelegate setConfidences:confidences];
    [_drawDelegate setViewWidth:viewWidth height:viewHeight];
}

@implementation VROBodyRecognitionDrawDelegate {
    std::vector<VROVector3f> _labelPositions;
    std::vector<NSString *> _labels;
    std::vector<VROBoundingBox> _boxes;
    std::vector<UIColor *> _colors;
    std::vector<float> _confidences;
    CGRect _dynamicCropBox;
    int _viewWidth, _viewHeight;
}

- (id)init {
    self = [super init];
    if (self) {
        _dynamicCropBox = CGRectNull;
    }
    return self;
}

- (void)drawRect {
    CGContextRef context = UIGraphicsGetCurrentContext();
    UIFont *font = [UIFont boldSystemFontOfSize:16];
    
    CGContextSetRGBFillColor(context, 0, 1, 0, 1);
    CGContextSetRGBStrokeColor(context, 0, 1, 0, 1);
    CGContextSetLineWidth(context, 3);
    
    if (!CGRectIsNull(_dynamicCropBox)) {
        CGFloat red = 0.0, green = 1.0, blue = 0.0, alpha = 1.0;
        CGContextSetRGBStrokeColor(context, red, green, blue, alpha);
        
        CGContextAddRect(context, CGRectApplyAffineTransform(_dynamicCropBox, CGAffineTransformMakeScale(_viewWidth, _viewHeight)));
        CGContextStrokePath(context);
    }
    
    if (_confidences.size() > 0) {
        for (std::pair<int, int> boneIndices : kSkeleton) {
            if (_confidences[boneIndices.first] < kConfidenceThreshold ||
                _confidences[boneIndices.second] < kConfidenceThreshold) {
                continue;
            }
            
            CGFloat red = 0.0, green = 0.0, blue = 0.0, alpha = 0.0;
            [_colors[boneIndices.first] getRed:&red green:&green blue:&blue alpha:&alpha];
            CGContextSetRGBStrokeColor(context, red, green, blue, 1);
            
            CGContextMoveToPoint(context, _labelPositions[boneIndices.first].x, _labelPositions[boneIndices.first].y);
            CGContextAddLineToPoint(context, _labelPositions[boneIndices.second].x, _labelPositions[boneIndices.second].y);
            CGContextStrokePath(context);
        }
    }
    
    for (int i = 0; i < _labels.size(); i++) {
        if (_confidences[i] < kConfidenceThreshold) {
            continue;
        }
        VROVector3f point = _labelPositions[i];
        
        CGFloat red = 0.0, green = 0.0, blue = 0.0, alpha =0.0;
        [_colors[i] getRed:&red green:&green blue:&blue alpha:&alpha];
        
        CGContextSetRGBFillColor(context, red, green, blue, 1);
        CGContextSetRGBStrokeColor(context, 0, 0, 0, 1);
        CGContextSetLineWidth(context, 1);
        
        float radius = 5;
        CGRect rect = CGRectMake(point.x - radius, point.y - radius, radius * 2, radius * 2);
        CGContextAddEllipseInRect(context, rect);
        CGContextDrawPath(context, kCGPathFillStroke);
        
        if (kDrawLabels) {
            [_labels[i] drawAtPoint:CGPointMake( point.x, point.y ) withAttributes:@{ NSFontAttributeName:font,
    NSForegroundColorAttributeName : _colors[i] } ];
        }
    }
}

- (void)setLabels:(std::vector<NSString *>)labels positions:(std::vector<VROVector3f>)positions {
    _labels = labels;
    _labelPositions = positions;
}
- (void)setBoxes:(std::vector<VROBoundingBox>)boxes {
    _boxes = boxes;
}
- (void)setColors:(std::vector<UIColor *>)colors {
    _colors = colors;
}
- (void)setConfidences:(std::vector<float>)confidences {
    _confidences = confidences;
}
- (void)setDynamicCropBox:(CGRect)box {
    _dynamicCropBox = box;
}
- (void)setViewWidth:(int)width height:(int)height {
    _viewWidth = width;
    _viewHeight = height;
}

@end
