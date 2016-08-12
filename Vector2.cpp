#include "FreeformTool.h"
#include "Vector2.h"

const Vector2 Vector2::zero = Vector2(0.0f, 0.0f);
const Vector2 Vector2::unitX = Vector2(1.0f, 0.0f);
const Vector2 Vector2::unitY = Vector2(0.0f, 1.0f);
const Vector2 Vector2::one = Vector2(1.0f, 1.0f);

Vector2 Vector2::rotate(float radians) const
{
	float sin = std::sin(radians);
	float cos = std::cos(radians);
	return Vector2(x * cos - y * sin, x * sin + y * cos);
}

float Vector2::angleTo(Vector2& b) const
{
	ME_ASSERT(isEqual(norm2(), 1.0f, 1e-4f));
	ME_ASSERT(isEqual(b.norm2(), 1.0f, 1e-4f));
	float sin = cross(b);
	float cos = dot(b);
	return std::atan2(sin, cos);
}

