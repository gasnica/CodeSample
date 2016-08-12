#pragma once

#include <objidl.h> // needed for HDC
#include <gdiplus.h> // needed for Gdiplus::pen

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This is minimal drawing utility. 

Currently it supports drawing FreeformLines, and elements of ArcSpline, namely
Arcs and Segments. It also marks arc ends with x-marks, and frames each detected
corner point in a rectangle.

drawFreeformLine() supports drawing only a parital line, which helps keeping
interactive frame rates while mouse-drawing.

See: ArcSpline, FreeformLine, main
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

class ArcSpline;
class TweakUtil;
class FreeformLine;
struct Vector2;

// This draws various shapes on a GDI+ device context.
class ShapeDrawer
{
public:
	// Support two modes: partial and full drawing
	enum Mode { MODE_FAST_AND_PARTIAL, MODE_SLOW_BUT_DOUBLEBUFFERED };

	// Construct Graphics & optional Bitmap objects for the given device context
	ShapeDrawer(HDC hdc, Mode mode);

	// Destroy temporary graphics & bitmap objects
	~ShapeDrawer();

	// Fill the context with color
	void clear(const Gdiplus::Color& color);

	// Fancy draw-point, which draws either a cross or a bounding box for the point
	void drawPoint(const Vector2& point, bool drawCross = false, Gdiplus::Pen* pen = nullptr, float halfSize = 2);

	// Draw a line
	inline void drawLine(const Vector2& a, const Vector2& b, Gdiplus::Pen* pen = nullptr);

	// Draw a FreeformLine
	int drawFreeformLine(const FreeformLine& line, int startAt = 0, Gdiplus::Pen* pen = nullptr);

	// Draw ArcSpline, including segments, arcs, element endpoints, corner points
	void drawArcSpline(const ArcSpline& spline, float brushWidth = 2.0f);

	// Draw TweakUtil panel
	void drawTweakUtil(const TweakUtil& util);

private:
	// Current device context
	const HDC hdc;

	// Bitmap used for double-buffered drawing
	Gdiplus::Bitmap* bitmap;

	// Graphics object to draw with
	Gdiplus::Graphics* graphics;

	// Default pen used by some of the functions 
	Gdiplus::Pen blackPen;
};
