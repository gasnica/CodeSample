#pragma once

#include <vector>

#include "Common.h"
#include "Vector2.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This file holds basic geometric shapes Line, Circle, Biarc. It also has
LineOrCircle which is used to store cached sub-shapes in Biarc.

Each shape implements the signeDistTo(point). It's used for querying error
between a shape & an input FreeformLine.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


// Infinite geometric line 
struct Line
{
	// Create a Line from a point on it & the normal vector
	static Line fromPointAndNormal(const Vector2& point, const Vector2& normal);

	// Create a Line passing through both points
	static Line between(const Vector2& p0, const Vector2& p1) { return Line::fromPointAndNormal(p0, p0.directionTo(p1).rotate90()); }

	// Normal direction of the line
	const Vector2 normal() const { return Vector2(a, b); } // Normal of the line

	// Returns signed distance between a point & the line
	float signedDistTo(const Vector2& point) const { return normal().dot(point) + c; }

	// Returns projection of a point on to the line
	Vector2 project(const Vector2& point) const { return point - normal() * signedDistTo(point); }

	// Store canonical form of the line ax + by + c = 0;
	float a, b, c;
};


// Geometric circle
struct Circle
{
	// Circle center
	Vector2 center() const { return Vector2(x, y); }

	// Return signed distance between a point & the circle
	float signedDistTo(const Vector2& point) const { return (point - center()).norm() - radius; }

	// Store circle center and radius
	float x, y, radius;
};


// Holds shape type and a union that can hold either a Circle or a Line
struct CircleOrLine
{
	// Create a Circle or a Line that best fits the constraints: 2 points on the shape & a tangent at the first point
	static CircleOrLine* fitCircleOrLine(const Vector2& point0, const Vector2& tangent0, const Vector2& point1, CircleOrLine* result);

	// Initialize new CircleOrLine to 'invalid' shape
	CircleOrLine() : type(TYPE_INVALID) { }

	// Represent a circle
	void setCircle(const Circle& circle) { this->circle = circle; type = TYPE_CIRCLE; }

	// Represent a line
	void setLine(const Line& line) { this->line = line; type = TYPE_LINE; }

	// Return signed distance between a point & the represented shape
	float signedDistTo(const Vector2& point) const { switch (type) { case TYPE_CIRCLE: return circle.signedDistTo(point); case TYPE_LINE: return line.signedDistTo(point); }; ME_ASSERT(false); return ME_A_LOT; }

	// Determines represented shape
	enum { TYPE_INVALID = ME_MUST_BE_ZERO, TYPE_CIRCLE, TYPE_LINE } type;

	// Shape storage
	union { Circle circle; Line line; };
};


// Geometric biarc 
//
// Biarc has to be initialized with points, tangents, and both d0 & d1 parameters.
// Then you need to run calcCachedShapes() before calling signedDistFrom()
//
// This simplified implementation only works for positive d0, d1 parameter, or when d1 == 0
// 
// This simplified implementation only measures distance from points to _circles_, 
// and _lines_ that define the child arc & segment shapes, which can lead to
// incorrect results.
// 
// From http://www.ryanjuckett.com/programming/biarc-interpolation/
struct Biarc
{
	// A pair of parameters: d0, d1
	struct DParam { float d0, d1; };

	// Calculate a set of possible d0, d1 parameters for given start/end points & tangents.
	// 
	// Results are generated for a series of 'r' parameters (r defines d0/d1 ratio, to give a unique solution).
	// numResults of 'r' values are generated in a geometric series.
	//
	// Additionally a degenerate biarc with a single arc & tangent discontinuity at end can be generated when addSingleArcResult is true.
	static void findPossibleBiarcParams(const Biarc& biarcPointsAndTangents, float rLower, float rUpper, int numResults, bool addSingleArcResult, std::vector<DParam>* result);

	// Cache child shapes for faster signedDistFrom computation
	void calcCachedShapes(); 

	// 'Mid-point' of the biarcs, where two child arcs meet
	Vector2 midPoint() const { return Vector2::interpolate(q0(), q1(), param.d0/(param.d0+param.d1+FLT_MIN)); }

	// Tangent at the mid-point
	Vector2 midTangent() const { ME_ASSERT(0.0f <= param.d0 || 0.0f == param.d1); return q0().directionTo(q1()); }

	// Internal helper control points
	Vector2 q0() const { return point0 + tangent0 * param.d0; }
	Vector2 q1() const { return point1 - tangent1 * param.d1; }

	// Signed distance from the biarc curve
	float signedDistTo(const Vector2& point) const;

	// Start & end points
	Vector2 point0, tangent0;

	// Tangents at the start & end points
	Vector2 point1, tangent1;

	// Parameters determining the radii and shape of the arcs
	DParam param;

	// Cached helper line used to determine which arc of the Biarc a point should be projected onto
	Line divLine; 

	// Cached circles/lines that approximate the child arcs/segments
	CircleOrLine shape0, shape1; 
};
