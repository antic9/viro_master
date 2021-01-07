//
//  VROKnuthPlassFormatter.hpp
//  ViroRenderer
//
//  Created by Raj Advani on 12/2/16.
//  Copyright © 2016 Viro Media. All rights reserved.
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

#ifndef VROKnuthPlassFormatter_hpp
#define VROKnuthPlassFormatter_hpp

#include <stdio.h>
#include <string>
#include <vector>
#include <list>
#include <memory>

extern int kInfinity;

enum class KPNodeType {
    Glue,
    Box,
    Penalty
};

struct KPSum {
    float width, stretch, shrink;
    
    KPSum() : width(0), stretch(0), shrink(0) {}
    KPSum(float width, float stretch, float shrink) : width(width), stretch(stretch), shrink(shrink) {}
    virtual ~KPSum() {}
};

inline KPSum operator-(KPSum lhs, const KPSum& rhs) {
    lhs.width -= rhs.width;
    lhs.stretch -= rhs.stretch;
    lhs.shrink -= rhs.shrink;
    return lhs;
}

struct KPNode {
    KPNodeType type;
    std::wstring value;
    
    KPNode(KPNodeType type, std::wstring value) : type(type), value(value) {}
    virtual ~KPNode() {}
};

struct KPGlue : public KPNode {
    float width, stretch, shrink;
    
    KPGlue(float width, float stretch, float shrink, std::wstring value) : KPNode(KPNodeType::Glue, value),
    width(width), stretch(stretch), shrink(shrink) {}
    virtual ~KPGlue() {}
};

struct KPBox : public KPNode {
    float width;
    
    KPBox(float width, std::wstring value) : KPNode(KPNodeType::Box, value),
    width(width) {}
    virtual ~KPBox() {}
};

struct KPPenalty : public KPNode {
    float width, penalty;
    float flagged;
    
    KPPenalty(float width, float penalty, float flagged) : KPNode(KPNodeType::Penalty, L""),
    width(width), penalty(penalty), flagged(flagged) {}
    virtual ~KPPenalty() {}
};

struct KPBreakpoint {
    int position;
    int demerits;
    float ratio;
    int line;
    int fitnessClass;
    KPSum totals;
    std::shared_ptr<KPBreakpoint> previous;
    
    KPBreakpoint(int position, int demerits, float ratio, int line, int fitnessClass, KPSum sum, std::shared_ptr<KPBreakpoint> previous) :
    position(position), demerits(demerits), ratio(ratio), line(line), fitnessClass(fitnessClass), totals(sum), previous(previous) {}
    virtual ~KPBreakpoint() {}
};

struct KPBreakpointCandidate {
    std::shared_ptr<KPBreakpoint> parent;
    int demerits;
    float ratio;
    
    KPBreakpointCandidate(int demerits) : demerits(demerits), ratio(0) {}
    KPBreakpointCandidate(std::shared_ptr<KPBreakpoint> parent, int demerits, float ratio) :
    parent(parent), demerits(demerits), ratio(ratio) {}
    virtual ~KPBreakpointCandidate() {}
};

struct VROBreakpoint {
    int position;
    float ratio;
    VROBreakpoint(int position, float ratio) : position(position), ratio(ratio) {}
    virtual ~VROBreakpoint() {}
};

struct KPDemerits {
    int line;
    int flagged;
    int fitness;
};

struct KPOptions {
    KPDemerits demerits;
    int tolerance;
};

/*
 Formats (justifies) text according to the Knuth Plass dynamic programming algorithm.
 See here for details on the algorithm: http://defoe.sourceforge.net/folio/knuth-plass.html
 */
class VROKnuthPlassFormatter {
    
public:
    
    VROKnuthPlassFormatter(std::vector<std::shared_ptr<KPNode>> &nodes, std::vector<float> &lineLengths,
                           float tolerance);
    std::vector<VROBreakpoint> run();
    
private:
    
    std::vector<std::shared_ptr<KPNode>> _nodes;
    std::vector<float> _lineLengths;
    KPOptions _options;
    
    /*
     Find all the candidate breakpoints for the given node. There will be at
     most one candidate created per existing parent breakpoint. 
     
     The found candidates will be added to the breakpoint list. Existing breakpoints
     in the list that are no longer optimal will be removed from the list.
     */
    void findCandidateBreakpoints(std::shared_ptr<KPNode> &node, int nodeIndex, KPSum &sum,
                                  std::list<std::shared_ptr<KPBreakpoint>> &breakpoints) const;

    KPSum computeSum(const KPSum &sum, int breakpointIndex) const;
    float computeCost(const KPSum &sumFromParentToNode, std::shared_ptr<KPBreakpoint> &parent,
                      int currentLine) const;
    
};

#endif /* VROKnuthPlassFormatter_hpp */
