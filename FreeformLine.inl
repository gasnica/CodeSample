#pragma once

Vector2 FreeformLine::getPointAt(float t) const
{
	ME_ASSERT(points.size());
	auto it = points.upper_bound(t);
	const std::pair<const float, Vector2>& next = *it;
	const std::pair<const float, Vector2>& prev = *--it;
	float localT = (t - prev.first) / (next.first - prev.first);
	return Vector2::interpolate(prev.second, next.second, localT);
}

Vector2 FreeformLine::getTangentAt(float t) const
{
	ME_ASSERT(ME_EPSILON < halfSmoothingSpread);
	// The clip is not necessary in general. Here, it will freeze the tangent at 2.0f * halfSmoothingSpread before either end.
	float ta = getClipped(t - halfSmoothingSpread, clippingRange.start, clippingRange.end - clippingMargin);
	float tb = getClipped(t + halfSmoothingSpread, clippingRange.start + clippingMargin, clippingRange.end);
	Vector2 a = getPointAt(ta);
	Vector2 b = getPointAt(tb);
	return (b - a).normalized();
}
