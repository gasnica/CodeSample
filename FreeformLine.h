#pragma once

#include <vector>
#include <map>

#include "Vector2.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FreeformLine collects points, puts them in a map & assigns each point with
'distance traveled' from the beginning of the line. You can search a point
on the line by it's 't' (distance from beginning) param in. That should be 
in log(length) time, given the map is implemented as a balanced tree.

Querying local tangent is very noisy given our line is from a mouse input and
point-to-point distance often is just a pixel or a few. getTangentAt() returns
smoothed tangent querying points halfSmoothingSpread away on either side.  When
setTangentBounds() is used those those points are clipped to within a range. 
This is a trick to freeze line's measured tangent when close to clipping bounds.

You can serialize a FreeformLine to a text file with the stream operators.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

// Converts a series of input points in to a parametrized line.
//
// Also queries smoothed tangent along the line & lets you restrict tangent
// calculation to a Range, which helps when working with a section of a line
// only.
class FreeformLine : public RefCounted
{
public:
	FreeformLine() : halfSmoothingSpread(10.0f), cachedLength(0.0f), clippingRange({-ME_A_LOT, ME_A_LOT}), clippingMargin(0.0f)  { }

	// Append a point to the line, grow it's length.
	void addPoint(const Vector2& point);
	
	// Return total length of this line
	float length() const { return cachedLength;  } 

	// Calculate the point on the line at 't' distance from it's start.
	inline Vector2 getPointAt(float t) const;

	// Calculate approximate smoothed tangent at 't' distance from the line's start; 't' is clipped to within getBounds()
	inline Vector2 getTangentAt(float t) const;

	// Get clipping bounds used for tangent calculation
	const Range& getBounds() const { return clippingRange; }

	// Set clipping bounds for tangent calculation
	void setBounds(const Range& range);

	// Determines distance between points used to query the tangent at a point. Must be greater than epsilon.
	float halfSmoothingSpread; 

	// Serialize & deserialize the FreeformLine
	friend std::ostream& operator << (std::ostream& stream, const FreeformLine& line);
	friend std::istream& operator >> (std::istream& stream, FreeformLine& line);

	// Allow drawing using original input points
	friend class ShapeDrawer;

protected:
	// Maps distance along the line (t) to the corresponding line input point
	std::map<float, Vector2> points; 

	// FreeformLine's length
	float cachedLength;

	// This is temp processing state and is not serialized.
	//
	// Restricts tangent calculations to data withing a range. This allows computing tangents on sub-section of line between two 'corners' or tangent-discontinuity points.
	Range clippingRange; 
	float clippingMargin;
};

#include "FreeformLine.inl"
