#include "FreeformTool.h"
#include "FreeformLine.h"

#include <cmath>
#include <iostream>
#include <vector>

#include "Common.h"

void FreeformLine::addPoint(const Vector2& point)
{
	if (points.size())
	{
		const std::pair<const float, Vector2>& lastPoint = *++points.rbegin();
		float length = lastPoint.first + lastPoint.second.distTo(point);
		points[length] = point;
		points.rbegin()->second = point;
		cachedLength = length;
	}
	else
	{
		points[ME_A_LOT] = points[0.0f] = points[-ME_A_LOT] = point;
	}
}

void FreeformLine::setBounds(const Range& range)
{
	clippingRange = range;
	clippingMargin = std::fmin(2.0f * halfSmoothingSpread, range.length());
}

std::ostream& operator<<(std::ostream& stream, const FreeformLine& line)
{
	stream << line.halfSmoothingSpread << " ";
	stream << (int)line.points.size() << " ";
	for (const auto& pt : line.points) { stream << pt.first << " " << pt.second.x << " " << pt.second.y << " "; }
	return stream;
}

std::istream& operator>>(std::istream& stream, FreeformLine& line)
{
	int numPoints;
	float t;
	Vector2 v;

	line.points.clear();
	stream >> line.halfSmoothingSpread;
	stream >> numPoints;
	for (int i = 0; i < numPoints; i++)
	{
		stream >> t >> v.x >> v.y;
		line.points[t] = v;
	}
	line.cachedLength = line.points.size() ? (++line.points.rbegin())->first : 0.0f;
	return stream;
}
