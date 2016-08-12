#pragma once

#include <vector>

#include "ArcSplineUtil.h" // for input struct
#include "Geometry.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Please, note that ArcSpline generation has been tweaked to work well
for mouse drawing on screen, where a unit length corresponds to one pixel.

The algorithm works well with lines that span over hundreds of units, and have
local curvatures of several units & more preferably tens of units.

Consider always scaling FreeformLine input to it's visible on-screen size to get
consistent drawing behavior & user experience.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


class FreeformLine;
class SplineElement;

// Holds reference to the source/input FreeformLine & the resulting list of
// SplineElements (Arcs & Segments). Additionally it keeps a list of debugCorners
// to mark points of tangent discontinuity, and a reference to ProcessingInput
// used to generate the spline.
class ArcSpline : public RefCounted
{
public:
	ArcSpline(const FreeformLine* line, ArcSplineUtil::ProcessingInput* processingInput = new ArcSplineUtil::ProcessingInput());

	// Recalculate the spline with updated processingInput
	void recreateSpline(ArcSplineUtil::ProcessingInput* processingInput = nullptr);

	// Input FreeformLine
	const ref<const FreeformLine> sourceLine;

	// Algorithm parameters used for computing the spline
	ref<ArcSplineUtil::ProcessingInput> processingInput;

	// Elements forming the spline
	std::vector<ref<SplineElement>> displayShapes; 

	// List of corners, where tangent continuity is broken. This is purely for displaying.
	std::vector<Vector2> debugCorners;

protected:

	// Identify corners and segments
	void findCornersAndSegments(FreeformLine& line, std::vector<Range>* result);

	// Convert non-segment line sections into biarc-splines & convert all resulting geometric shapes into SplineElements.
	void generateBiarcsAndFinalShapes(FreeformLine& line, std::vector<Range>* mutableCornersAndSegments, std::vector<Vector2>* outCorners, std::vector<ref<SplineElement>>* outDisplayShapes);
};


// Base class for elements of ArcSpline.
// Implement distTo() & distToEndPoint() methods to allow click-selecting.
class SplineElement : public RefCounted
{
public:
	// Types of SplineElement
	enum Type { TYPE_INVALID = ME_MUST_BE_ZERO, TYPE_ARC, TYPE_SEGMENT } const type;

	// Dist from the shape to the point
	virtual float distTo(const Vector2& point) const { return FLT_MAX; }

	// Minimum dist from either of the endpoints of the shape to the specified point
	virtual float distToEndPoint(const Vector2& point) const { return FLT_MAX; }

protected:
	SplineElement(Type type) : type(type) { } // not a final class

private:
	SplineElement() : type(TYPE_INVALID) { } // disallow
};


// Arc shape forming the ArcSpline
class SplineArc : public SplineElement
{
public:
	SplineArc(const Circle& circle, const Vector2& p0, const Vector2& tangentAtP0, const Vector2& p1, int idx = -1);

	// Dist from the shape to the point
	virtual float distTo(const Vector2& point) const;

	// Minimum dist from either of the endpoints of the shape to the specified point
	virtual float distToEndPoint(const Vector2& point) const;

	// The circle that defines the arc
	Circle circle;

	// Start and end angles of the arc
	float startAngle, sweepAngle;

	// Index indicating whether this is the starting or ending arc in the original biarc. Only used for drawing.
	int idxInBiarc;
};


// Segment shape forming the ArcSpline
class SplineSegment : public SplineElement
{
public:
	SplineSegment(const Vector2& p0, const Vector2& p1, int idx = -1) : SplineElement(TYPE_SEGMENT), p0(p0), p1(p1), idxInBiarc(idx) { }

	// Dist from the shape to the point
	virtual float distTo(const Vector2& point) const;

	// Minimum dist from either of the endpoints of the shape to the specified point
	virtual float distToEndPoint(const Vector2& point) const;

	// Segment endpoints
	Vector2 p0, p1;

	// Index indicating if this segment was identified independently, or is a part of a biarc. Only used for drawing.
	int idxInBiarc;
};