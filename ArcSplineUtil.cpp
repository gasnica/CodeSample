#include "FreeformTool.h"
#include "ArcSplineUtil.h"

#include "FreeformLine.h"
#include "Geometry.h"

void ArcSplineUtil::findCorners(const FreeformLine& line, const CornersInput& input, std::vector<Range>* result)
{
	const Range& tBounds = line.getBounds();
	Range cornerSection;

	const float margin = std::ceil(input.outerInterMeasurementFactor + 0.5f * input.innerInterMeasurementFactor);
	float d[4] = { -(input.outerInterMeasurementFactor + input.innerInterMeasurementFactor),
		-input.innerInterMeasurementFactor,
		input.innerInterMeasurementFactor,
		(input.innerInterMeasurementFactor + input.outerInterMeasurementFactor) };
	for (float& di : d) { di *= line.halfSmoothingSpread; }

	for (float t = tBounds.start + margin; t <= tBounds.end - margin; t += input.tStep)
	{
		// hack:
		Vector2 tangents[] = { line.getTangentAt(t + d[0]), line.getTangentAt(t + d[1]),
			line.getTangentAt(t + d[2]), line.getTangentAt(t + d[3]) };
		float angles[3] = { std::fabs(tangents[0].angleTo(tangents[1])) * ME_RAD_TO_DEG,
			std::fabs(tangents[1].angleTo(tangents[2])) * ME_RAD_TO_DEG,
			std::fabs(tangents[2].angleTo(tangents[3])) * ME_RAD_TO_DEG };
		if (angles[0] < input.outerMaxAngleInDeg && // not needed?
			angles[1] > input.innerMinAngleInDeg &&
			angles[2] < input.outerMaxAngleInDeg && // not needed?
			angles[0] / angles[1] < 1.0f / 3.0f &&
			angles[2] / angles[1] < 1.0f / 3.0f)
		{
			// store corner marking temporarily
			cornerSection.include(t);
		}
		else if (cornerSection.length() >= input.minNumberTestPositivesInSeries)
		{
			// Check if the previous corner 'section' was close enough, if so, merge it with the current one.
			if (result->size() && result->back().end + input.maxDistBetweenCornersToMerge >= cornerSection.start)
			{
				result->back().end = cornerSection.end;
			}
			else
			{
				result->push_back(cornerSection);
			}
			cornerSection.invalidate();
		}
		else
		{
			cornerSection.invalidate();
		}
	}

	// For each corner section, find the best point to represent that corner
	for (Range& c : *result)
	{
		Vector2 tangent0 = line.getTangentAt(c.start + d[1]);
		Vector2 tangent1 = line.getTangentAt(c.end + d[2]);
		Vector2 searchDir = tangent0 - tangent1;
		float tBest = 0.5f * (c.start + c.end);

		if (searchDir.norm2() > ME_EPSILON2)
		{
			float furthestPosAlongDir = -FLT_MAX;
			// Allow the corner to drift past the original limits (this is needed for series of segments of length close to line's halfSmoothingSpread
			c.inflate(2.0f * input.innerInterMeasurementFactor * line.halfSmoothingSpread);
			// find point that's furthest along the search direction
			for (float t = c.start; t <= c.end; t++)
			{
				float posAlongDir = searchDir.dot(line.getPointAt(t));
				if (furthestPosAlongDir < posAlongDir)
				{
					tBest = t;
					furthestPosAlongDir = posAlongDir;
				}
			}
		}

		c = { tBest, tBest };
	}
}

bool ArcSplineUtil::isSegment(const FreeformLine& line, const Range segmentBounds, const SegmentsInput& input, float* meanError2)
{
	ME_ASSERT(segmentBounds.start >= 0.0f && segmentBounds.end <= line.length());
	Vector2 p0 = line.getPointAt(segmentBounds.start);
	Vector2 p1 = line.getPointAt(segmentBounds.end);
	Line testFitLine = Line::between(p0, p1);
	float dist = p0.distTo(p1);
	*meanError2 = ArcSplineUtil::calcMeanSquaredError(line, segmentBounds.start, input.tStep, segmentBounds.end, testFitLine);
	float limitMultiplier = (dist / input.referenceSegmentLength);
	return *meanError2 <= input.maxMeanErrorAtReferenceLength * input.maxMeanErrorAtReferenceLength * limitMultiplier;
}

void ArcSplineUtil::convertLineToBiarcs(const FreeformLine& line, const BiarcsInput& input, std::vector<Biarc>* result)
{
	/*

	THIS FUNCTION IS NOT SHARED IN THIS CODE SAMPLE

	*/
}

template <class TShape> float ArcSplineUtil::calcMeanSquaredError(const FreeformLine& line, float tStart, float tStep, float tEnd, const TShape& fittingShape)
{
	float numMeasurements = FLT_MIN;
	float sumError2 = 0.0f;

	for (float tCurr = tStart; tCurr < tEnd; tCurr += tStep, numMeasurements += 1.0f)
	{
		Vector2 pointOnLine = line.getPointAt(tCurr);
		float signedDist = fittingShape.signedDistTo(pointOnLine);
		sumError2 += signedDist * signedDist;
	}

	return sumError2 / numMeasurements;
}

float ArcSplineUtil::minDistToBiarcMidPoint(const FreeformLine& line, float tStart, float tStep, float tEnd, const Biarc& biarc)
{
	Vector2 midPoint = biarc.midPoint();
	if (biarc.param.d1 == 0.0f && biarc.shape0.type == CircleOrLine::TYPE_CIRCLE)
	{
		// handle single arcs also
		const Circle& c = biarc.shape0.circle;
		midPoint = (biarc.point0 + biarc.point1 - 2.0f * c.center()).normalized() * c.radius;
		if (0 <= (biarc.point1 - biarc.point0).dot(biarc.tangent0))
		{
			midPoint = c.center() + midPoint;
		}
		else
		{
			midPoint = c.center() - midPoint;
		}
	}
	float minDist2 = FLT_MAX;
	for (float tCurr = tStart; tCurr < tEnd; tCurr += tStep)
	{
		Vector2 pointOnLine = line.getPointAt(tCurr);
		float dist2 = (pointOnLine - midPoint).norm2();
		if (dist2 < minDist2) { minDist2 = dist2; }
	}
	return std::sqrt(minDist2);
}
