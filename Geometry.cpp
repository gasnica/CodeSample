#include "FreeformTool.h"
#include "Geometry.h"

#include <cmath>

#include "Common.h"

Line Line::fromPointAndNormal(const Vector2& point, const Vector2& normal)
{
	ME_ASSERT(isEqual(normal.norm2(), 1.0f, 1e-4f));
	Line result;
	result.a = normal.x;
	result.b = normal.y;
	result.c = -normal.dot(point);
	return result;
}

CircleOrLine* CircleOrLine::fitCircleOrLine(const Vector2& point0, const Vector2& tangent0, const Vector2& point1, CircleOrLine* result)
{
	result->type = CircleOrLine::TYPE_INVALID;

	const float pNorm2 = (point1 - point0).norm2();
	if (pNorm2 > ME_EPSILON2)
	{
		Line line0 = Line::fromPointAndNormal(point0, -tangent0);

		Vector2 mid = (point0 + point1) * 0.5f;
		Vector2 normal1 = (point1 - point0).rotate90().normalized();

		float dist = line0.signedDistTo(mid);

		Vector2 proj = line0.project(mid);
		Vector2 lead = proj - point0;
		if (lead.norm2() > ME_EPSILON2)
		{
			Vector2 center = proj + lead * (dist * dist / lead.norm2());
			float radius = std::sqrt((center - point0).norm2());
			if (radius <= ME_MAX_ARC_RADIUS && radius * radius < ME_MAX_ARC_RADIUS_TO_CHORD_LENGTH_RATIO * ME_MAX_ARC_RADIUS_TO_CHORD_LENGTH_RATIO * pNorm2)
			{
				result->setCircle({ center.x, center.y, radius });
			}
		}

		if (!result->type)
		{
			//return straight line
			result->setLine(Line::between(point0, point1));
		}
	}
	else
	{
		// Fitting a zero circle
		result->setCircle({ point0.x, point0.y, 0.0 });
	}

	ME_ASSERT(std::fabs(result->signedDistTo(point0)) < ME_MAX_SPLINE_GAP);
	ME_ASSERT(std::fabs(result->signedDistTo(point1)) < ME_MAX_SPLINE_GAP);

	return result;
}

void Biarc::findPossibleBiarcParams(const Biarc& biarcPointsAndTangents, float rLower, float rUpper, int numResults, bool addSingleArcResult, std::vector<Biarc::DParam>* result)
{
	const Vector2& t0 = biarcPointsAndTangents.tangent0;
	const Vector2& t1 = biarcPointsAndTangents.tangent1;
	const Vector2 v = biarcPointsAndTangents.point1 - biarcPointsAndTangents.point0;

	ME_ASSERT(result->empty());
	float rIterationMultiplier = numResults > 1 ? std::powf(rUpper / rLower, 1.0f / (numResults - 1 + ME_EPSILON)) : 1.0f;
	float r = numResults > 1 ? rLower : 1.0f; // biarc length ratio parameter, if querying for one result only then override r = 1.0f
	for (int i = 0; i < numResults; ++i, r *= rIterationMultiplier)
	{
		const Vector2 t = r * t0 + t1;
		float d1 = -1.0f; // mark no result found

		//  a * x ^ 2 + b * x + c = 0 / solve
		const float a = r * (1.0f - t0.dot(t1));
		const float b = v.dot(t);
		const float c = -0.5f * v.dot(v);
		const float delta = b * b - 4.0f * a * c;

		bool foundSolution = false;
		if (std::fabs(a) > ME_EPSILON)
		{
			if (delta >= 0.0f)
			{
				const float deltaSqrt = std::sqrt(delta);
				const float sol0 = (-b - deltaSqrt) / (2.0f * a);
				const float sol1 = (-b + deltaSqrt) / (2.0f * a);
				d1 = std::fmax(sol0, sol1);
			}
			if (delta < 0.0f || d1 < 0.0f) { ME_ASSERT(false); continue; } // Can't figure the biarc, safe exit
			if (d1 < ME_MAX_D_PARAM) { foundSolution = true; }
		}

		if (!foundSolution && std::fabs(v.dot(t1)) > FLT_MIN)
		{
			// Tangents are parallel
			d1 = -c / b;
			if (d1 < ME_MAX_D_PARAM) { foundSolution = true; }
		}

		ME_ASSERT(foundSolution); // Extremely rare case: Can't figure biarc
		if (ME_EPSILON < d1) { result->push_back(DParam{ r * d1, d1 }); }
	}

	// special cases for single-arc biarc. 
	if (addSingleArcResult)
	{
		// Set d1 = 0.0f, create tangent discontinuity at the end; use only for the last biarc in a FreeformLine section.
		float d0 = v.dot(v) / (2.0f * v.dot(t0));

		// Negative d0 allows for 180+ deg arcs
		if (!isEqual(0.0f, d0)) { result->push_back(DParam{ d0, 0.0f }); }
	}
}

void Biarc::calcCachedShapes()
{
	// Attempt to create helper objects for projections
	Vector2 midPoint = this->midPoint();
	CircleOrLine::fitCircleOrLine(point0, tangent0, midPoint, &shape0);
	CircleOrLine::fitCircleOrLine(point1, tangent1, midPoint, &shape1);
	ME_ASSERT(shape0.type && shape1.type);

	// helper for projection
	divLine = Line::fromPointAndNormal(midPoint, midTangent());
}

float Biarc::signedDistTo(const Vector2& point) const
{
	ME_ASSERT(0.0f <= param.d1);
	float sign = divLine.signedDistTo(point0) * divLine.signedDistTo(point);
	sign *= param.d1; // use the first shape if the second shape is degenerate (zero-radius circle)
	return (sign >= 0.0f ? shape0 : shape1).signedDistTo(point);
}
