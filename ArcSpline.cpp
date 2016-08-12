#include "FreeformTool.h"
#include "ArcSpline.h"

#include <algorithm>

#include "FreeformLine.h"
#include "Geometry.h"
#include "ArcSplineUtil.h"

ArcSpline::ArcSpline(const FreeformLine* line, ArcSplineUtil::ProcessingInput* processingInput /*= new ArcSplineUtil::ProcessingInput()*/) : sourceLine(line)
{
	recreateSpline(processingInput);
}

void ArcSpline::recreateSpline(ArcSplineUtil::ProcessingInput* processingInput /*= nullptr*/)
{
	if (processingInput) this->processingInput = processingInput;
	ME_ASSERT(this->processingInput);

	// Clear this
	debugCorners.clear();
	displayShapes.clear();

	// Make non-const copy of input
	FreeformLine lineCopy = *sourceLine;

	// Generate splines
	std::vector<Range> cornersAndSegments;
	findCornersAndSegments(lineCopy, &cornersAndSegments);
	generateBiarcsAndFinalShapes(lineCopy, &cornersAndSegments, &debugCorners, &displayShapes);
}

void ArcSpline::findCornersAndSegments(FreeformLine& line, std::vector<Range>* result)
{
	// Find all corners
	std::vector<Range> corners; corners.reserve(20);
	{
		Range fullLineBounds = { 0.0f, line.length() };
		line.setBounds(fullLineBounds);
		ArcSplineUtil::findCorners(line, processingInput->corners, &corners);
	}

	// For each two consecutive corners check if they can be connected by a segment.
	std::vector<Range> segments; segments.reserve(20);
	{
		corners.push_back(Range{ line.length(), line.length() }); // add a terminal
		float prevCorner = 0.0f;
		for (const Range& c : corners)
		{
			const Range segment = { prevCorner, c.start };
			float meanError2;
			if (ME_MAX_SPLINE_GAP < segment.length() && ArcSplineUtil::isSegment(line, segment, processingInput->segments, &meanError2))
			{
				segments.push_back(segment); 
			}
			prevCorner = c.end;
		}
		corners.pop_back(); // pop the terminal
	}

	// Sort segments and corners together
	//
	result->insert(result->end(), corners.begin(), corners.end());
	result->insert(result->end(), segments.begin(), segments.end());
	std::sort(result->begin(), result->end(), Range::isLess);
}

void ArcSpline::generateBiarcsAndFinalShapes(FreeformLine& line, std::vector<Range>* mutableCornersAndSegments, std::vector<Vector2>* outCorners, std::vector<ref<SplineElement>>* outDisplayShapes)
{
	// Add a terminal
	mutableCornersAndSegments->push_back(Range{ line.length(), line.length() + 1.0f });

	// Generate biarcs & put everything into a display-shape array
	//
	outCorners->reserve(20);
	outDisplayShapes->reserve(200);

	// Iterate through corners & segments combined into one list.
	//  - create display shapes for segments, 
	//  - convert non-segment sections into biarc splines & generate display shapes 
	std::vector<Biarc> biarcs; biarcs.reserve(20);
	Range prevMarker = { -1.0f, 0.0f };
	for (const Range& s : *mutableCornersAndSegments)
	{
		Range boundsBetweenMarkers = { prevMarker.end, s.start };
		ME_ASSERT(boundsBetweenMarkers.length() >= 0.0f);

		// Create biarc-splines between markers (corners) which are not connected by a segment
		if (boundsBetweenMarkers.length() > ME_MAX_SPLINE_GAP)
		{
			// Generate biarcs
			//
			processingInput->biarcs.tBounds = boundsBetweenMarkers;
			line.setBounds(boundsBetweenMarkers);
			biarcs.clear();
			ArcSplineUtil::convertLineToBiarcs(line, processingInput->biarcs, &biarcs);

			// Create display shapes for each sub-shapes of each Biarc
			// 
			ref<SplineElement> shape;
			for (const auto& b : biarcs)
			{
				if (ME_MAX_SPLINE_GAP <= b.point0.distTo(b.midPoint()))
				{
					switch (b.shape0.type)
					{
					case CircleOrLine::TYPE_CIRCLE: shape = new SplineArc(b.shape0.circle, b.point0, b.tangent0, b.midPoint(), 0); break;
					case CircleOrLine::TYPE_LINE: shape = new SplineSegment(b.point0, b.midPoint(), 0); break;
					}
					outDisplayShapes->push_back(shape);
				}
				if (ME_MAX_SPLINE_GAP <= b.point1.distTo(b.midPoint()))
				{
					switch (b.shape1.type)
					{
					case CircleOrLine::TYPE_CIRCLE: shape = new SplineArc(b.shape1.circle, b.midPoint(), b.midTangent(), b.point1, 1); break;
					case CircleOrLine::TYPE_LINE: shape = new SplineSegment(b.midPoint(), b.point1, 1); break;
					}
					outDisplayShapes->push_back(shape);
				}
			}
		}

		// Create display object for segments
		if (s.length() > ME_MAX_SPLINE_GAP)
		{
			ref<SplineElement> shape = new SplineSegment(line.getPointAt(s.start), line.getPointAt(s.end));
			outDisplayShapes->push_back(shape);
		}
		// Create display info for corners
		else if (s.length() == 0.0f)
		{
			outCorners->push_back(line.getPointAt(s.start));
		}
		prevMarker = s;
	}

	// Remove terminal
	mutableCornersAndSegments->pop_back();
}

SplineArc::SplineArc(const Circle& circle, const Vector2& p0, const Vector2& tangentAtP0, const Vector2& p1, int idx /*= -1*/) : SplineElement(SplineElement::TYPE_ARC), circle(circle), idxInBiarc(idx)
{
	const Vector2 arm0 = p0 - circle.center();
	const Vector2 arm1 = p1 - circle.center();
	startAngle = std::atan2f(arm0.y, arm0.x) * ME_RAD_TO_DEG;
	const float endAngle = std::atan2f(arm1.y, arm1.x) * ME_RAD_TO_DEG;
	sweepAngle = endAngle - startAngle;

	const float cross = arm0.normalized().cross(arm1.normalized());
	const float dot = arm0.normalized().dot(arm1.normalized());
	if (arm0.cross(tangentAtP0) * sweepAngle < 0.0f) { sweepAngle += sweepAngle < 0.0f ? 360.0f : -360.0f; }
}

float SplineArc::distTo(const Vector2& point) const
{
	// check if point is within the arc
	float endAngle = startAngle + sweepAngle;
	float start = std::fmin(startAngle, endAngle);
	float end = std::fmax(startAngle, endAngle);

	Vector2 arm = point - circle.center();
	float angle = std::atan2(arm.y, arm.x) * ME_RAD_TO_DEG;
	if (angle < start) { angle += 360.0f; }
	if (end < angle) { angle -= 360.0f; }

	bool isWithingAngles = start <= angle && angle <= end;

	if (isWithingAngles)
	{
		return std::fabs(arm.norm() - circle.radius);
	}
	else
	{
		return SplineArc::distToEndPoint(point);
	}
}

float SplineArc::distToEndPoint(const Vector2& point) const
{
	float endAngle = startAngle + sweepAngle;
	Vector2 p0 = circle.center() + (Vector2::unitX * circle.radius).rotate(startAngle * ME_DEG_TO_RAD);
	Vector2 p1 = circle.center() + (Vector2::unitX * circle.radius).rotate(endAngle * ME_DEG_TO_RAD);
	return std::fmin(point.distTo(p0), point.distTo(p1));
}

float SplineSegment::distTo(const Vector2& point) const
{
	Vector2 closestPoint;
	do
	{
		Vector2 u = point - p0;
		Vector2 v = p1 - p0;

		float c1 = u.dot(v);
		if (c1 <= 0.0f) { closestPoint = p0; break; }

		float c2 = v.dot(v);
		if (c2 <= c1) { closestPoint = p1; break; }

		float t = c1 / c2;
		closestPoint = p0 + t * v;
	} while (false);

	return closestPoint.distTo(point);
}

float SplineSegment::distToEndPoint(const Vector2& point) const
{
	return std::fmin(point.distTo(p0), point.distTo(p1));
}
