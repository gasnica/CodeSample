#pragma once

#include <cmath>

#include "Common.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Everyone needs one of those.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

struct Vector2;

struct Vector2Data // Used for unions
{
	float x, y;

	operator Vector2& () { return *(Vector2*)this; }
	operator const Vector2& () const { return *(Vector2*)this; }

	Vector2& vec() { return *(Vector2*)this; }
	const Vector2& vec() const { return *(Vector2*)this; }
};

struct Vector2 : Vector2Data
{
	Vector2() { x = y = 0.0f; }
	Vector2(float x, float y) { this->x = x; this->y = y; }
	Vector2(int x, int y) { this->x = float(x); this->y = float(y); }

	Vector2 operator + (const Vector2& b) const { return Vector2(x + b.x, y + b.y); }
	Vector2 operator - (const Vector2& b) const { return Vector2(x - b.x, y - b.y); }
	Vector2 operator * (float b) const { return Vector2(x * b, y * b); }
	Vector2 operator / (float b) const { return operator * (1.0f / b); }

	friend Vector2 operator * (float a, const Vector2& b) { return b * a; }
	Vector2 operator - () const { return Vector2(-x, -y); }

	Vector2& operator += (const Vector2& b) { x += b.x; y += b.y; return *this; }
	Vector2& operator -= (const Vector2& b) { x -= b.x; y -= b.y; return *this; }

	bool operator == (const Vector2& b) const { return x == b.x && y == b.y; }
	bool operator != (const Vector2& b) const { return x != b.x || y != b.y; }

	float& operator [] (int i) { ME_ASSERT((i & ~0x1) == 0); return (&x)[i]; }
	float operator [] (int i) const { ME_ASSERT((i & ~0x1) == 0); return (&x)[i]; }

	float dot(const Vector2& b) const { return x * b.x + y * b.y; }
	float cross(const Vector2& b) const { return x * b.y - y * b.x; }
	Vector2 scale(const Vector2 b) const { return Vector2(x * b.x, y * b.y); } // multiply corresponding components

	float norm() const { return std::sqrt(norm2()); }
	float norm2() const { return x * x + y * y; }
	float distTo(const Vector2& b) const { return operator - (b).norm(); }
	Vector2 normalized() const { return operator / (norm() + FLT_MIN); }
	Vector2 directionTo(const Vector2& b) const { return (b - *this).normalized(); }

	Vector2 rotate90() const { return Vector2(-y, x); }
	Vector2 rotate(float radians) const;
	float angleTo(Vector2& b) const;

	static Vector2 interpolate(const Vector2& a, const Vector2& b, float t) { return a * (1.0f - t) + b * t; }

	static const Vector2 zero;
	static const Vector2 unitX;
	static const Vector2 unitY;
	static const Vector2 one;
};


