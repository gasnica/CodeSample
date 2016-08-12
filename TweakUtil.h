#pragma once

#include <cwchar>
#include <vector>

#include "Common.h"

class ArcSpline;

// A utility to tweak float parameters; here, hardcoded to control chosen parameters of ArcSpline.
class TweakUtil
{
public:
	// Attach utility to a spline
	void attach(ArcSpline* spline, const Vector2& guiAnchorPoint);

	// Detach utility
	void detach();

	// When isAttached, update the parameters of referenced spline
	void update(const Vector2& guiMousePoint);

	// Is utility attached to a spline
	bool isAttached() const { return nullptr != spline; }

	// Has utility updated the spline since last attached.
	bool isActive() const { return wasUpdated; }

	// Allow private access for drawing
	friend class ShapeDrawer;

protected:
	// Spline to tweak
	ref<ArcSpline> spline = nullptr;

	// Center point of the tweaking panel
	Vector2 centerPoint;

	// Point on the tweaking panel, which corresponds to the original value from when utility was attached
	Vector2 initialPoint;

	// Current control point, driven by mouse, clipped to within the tweak panel, used to tweak values
	Vector2 currPoint;

	// Has utility updated the spline since last attached
	bool wasUpdated;

	// Dimensions of the tweak panel
	Vector2 halfSize;

	// Holds information on a parameter to tweak
	struct Tweakalbe
	{
		// Name/label for the parameter
		const wchar_t* label;

		// Pointer to the variable
		float * variable;

		// Reference value of the variable, corresponding to the center position on the tweak panel
		float referenceValue;

		// Value multiplier when the currPoint (control point) is at maximum position
		float valueMultiplierAtSliderMax;

		// Binds the parameter to x or y axis of the tweak panel
		int bindingAxis;
	};

	// List of values being tweaked simultaneously
	std::vector<Tweakalbe> tweakables;
};