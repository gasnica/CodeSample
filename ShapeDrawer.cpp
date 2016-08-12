#include "FreeformTool.h"
#include "ShapeDrawer.h"

#include <cwchar>
#include <gdiplus.h>

#include "ArcSpline.h"
#include "FreeformLine.h"
#include "TweakUtil.h"

Gdiplus::PointF toGdiPointF(const Vector2& v) { return Gdiplus::PointF(v.x, v.y); }

static std::pair<int, int> getCanvasSize(const HDC hdc);

ShapeDrawer::ShapeDrawer(HDC hdc, Mode mode) :
	hdc(hdc),
	bitmap(nullptr),
	graphics(nullptr),
	blackPen(Gdiplus::Color(255, 0, 0, 0))
{
	switch (mode)
	{
	case MODE_FAST_AND_PARTIAL:
		bitmap = nullptr;
		graphics = new Gdiplus::Graphics(hdc);
		break;
	case MODE_SLOW_BUT_DOUBLEBUFFERED:
		bitmap = new Gdiplus::Bitmap(getCanvasSize(hdc).first, getCanvasSize(hdc).second);
		graphics = new Gdiplus::Graphics(bitmap);
		break;
	}

	//graphics->SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeHighSpeed);
	graphics->SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	graphics->SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeHighQuality);
}

ShapeDrawer::~ShapeDrawer()
{
	// From http://www.codeproject.com/Articles/2055/Flicker-Free-Drawing-in-C
	if (bitmap) { Gdiplus::Graphics(hdc).DrawImage(bitmap, 0, 0); }
	delete graphics;
	delete bitmap;
}

void ShapeDrawer::clear(const Gdiplus::Color& color)
{
	graphics->Clear(color);
}

void ShapeDrawer::drawPoint(const Vector2& point, bool drawCross /*= false*/, Gdiplus::Pen* pen /*= nullptr*/, float halfSize /*= 2*/)
{
	float hs = halfSize; // rectangle half-size
	float x = point.x;
	float y = point.y;
	Gdiplus::PointF points[] = { { x + hs, y - hs }, { x + hs, y + hs }, { x - hs, y + hs }, { x - hs, y - hs }, { x + hs, y - hs } };
	if (drawCross)
	{
		graphics->DrawLine(pen?pen:&blackPen, points[0], points[2]);
		graphics->DrawLine(pen?pen:&blackPen, points[1], points[3]);
	}
	else
	{
		graphics->DrawLines(pen?pen:&blackPen, points, 5);
	}
}

void ShapeDrawer::drawLine(const Vector2& a, const Vector2& b, Gdiplus::Pen* pen /*= nullptr*/)
{
	graphics->DrawLine(pen?pen:&blackPen, toGdiPointF(a), toGdiPointF(b));
}

int ShapeDrawer::drawFreeformLine(const FreeformLine& line, int startAt /*= 0*/, Gdiplus::Pen* pen /*= nullptr*/)
{
	int paleGray = 196;
	Gdiplus::Pen      grayPen(Gdiplus::Color(255, paleGray, paleGray, paleGray));

	std::vector<Gdiplus::Point> points(line.points.size()-startAt);
	Gdiplus::Point* dst = &*points.begin();
	auto it = line.points.begin();
	for (int i = 0; i < startAt; i++, it++) {}
	for (; it != line.points.end(); it++) { *dst++ = Gdiplus::Point((int)it->second.x, (int)it->second.y); }
	graphics->DrawLines(pen ? pen : &grayPen, &points.front(), points.size());
	return line.points.size()-2;
}

void ShapeDrawer::drawArcSpline(const ArcSpline& spline, float brushWidth /*= 2.0f*/)
{
	Gdiplus::Pen      blackPen(Gdiplus::Color(255, 0, 0, 0)); blackPen.SetWidth(brushWidth);
	Gdiplus::Pen      crossPen(Gdiplus::Color(255, 150, 150, 150)); crossPen.SetWidth(2.0f);
	Gdiplus::Pen      grayPen(Gdiplus::Color(255, 150, 150, 150));
	Gdiplus::Pen      bluePen(Gdiplus::Color(255, 100, 100, 255)); bluePen.SetWidth(brushWidth);
	Gdiplus::Pen      redPen(Gdiplus::Color(255, 255, 0, 0)); redPen.SetWidth(brushWidth);

	const bool drawCross = true;
	for (const auto& v : spline.debugCorners) { drawPoint(v, !drawCross, &grayPen, 7); }

	Gdiplus::Pen* colors[] = { &blackPen, &bluePen, &redPen };

	for (SplineElement* shape : spline.displayShapes)
	{
		switch (shape->type)
		{
		case SplineElement::TYPE_SEGMENT:
			{
				const SplineSegment& segment = *static_cast<SplineSegment*>(shape);
				drawPoint(segment.p1, drawCross, &crossPen, 3);
				graphics->DrawLine(colors[segment.idxInBiarc+1], toGdiPointF(segment.p0), toGdiPointF(segment.p1));
			}
			break;
		case SplineElement::TYPE_ARC:
			{
				const SplineArc& arc = *static_cast<SplineArc*>(shape);
				Vector2 endPoint = arc.circle.center() + Vector2(arc.circle.radius, 0.0f).rotate((arc.startAngle + arc.sweepAngle) * ME_DEG_TO_RAD);
				drawPoint(endPoint, drawCross, &crossPen, 3);

				Gdiplus::RectF rect(toGdiPointF(arc.circle.center()), Gdiplus::SizeF()); rect.Inflate(arc.circle.radius, arc.circle.radius);
				graphics->DrawArc(colors[arc.idxInBiarc + 1], rect, arc.startAngle, arc.sweepAngle);
			}
			break;
		}
	}
}

void ShapeDrawer::drawTweakUtil(const TweakUtil& util)
{
	if (!util.isAttached() || !util.isActive()) { return; }

	Gdiplus::Pen      blackPen(Gdiplus::Color(255, 0, 0, 0));
	Gdiplus::Pen      grayPen(Gdiplus::Color(255, 200, 200, 200));

	// Draw bulged pattern
	const float relativeDisplacement = 0.1f;
	for (float r = 0.25f; r <= 1.0f; r += 0.25f)
	{
		Vector2 hs = util.halfSize * r;
		Vector2 points[] = { hs, hs.scale(Vector2(-1.0f, 1.0f)), -hs, hs.scale(Vector2(1.0f, -1.0f)), hs };
		const float halfSize = 0.5f * points[0].distTo(points[1]);
		// halfSide^2 + (r - relativeDisplacement * 2.0f * halfSide)^2 = r^2
		const float radius = halfSize * (1.0f + relativeDisplacement * relativeDisplacement) / 2.0f / (relativeDisplacement);

		for (int i = 0; i < 4; i++)
		{
			Vector2 center = Vector2::interpolate(points[i], points[i + 1], 0.5f) + points[i].directionTo(points[i + 1]).rotate90() * (radius - 1.0f *relativeDisplacement * halfSize);
			Vector2 arm0 = points[i] - center;
			Vector2 arm1 = points[i + 1] - center;
			float aStart = std::atan2f(arm0.y, arm0.x) * ME_RAD_TO_DEG;
			float aEnd = std::atan2f(arm1.y, arm1.x) * ME_RAD_TO_DEG;
			if (std::fabs(aStart - aEnd) > 180.0f) { aEnd += aStart < aEnd ? -360.0f : 360.0f; }

			center += util.centerPoint;
			Gdiplus::RectF rect(toGdiPointF(center), Gdiplus::SizeF()); rect.Inflate(toGdiPointF(Vector2(radius, radius)));
			graphics->DrawArc(r == 1.0f ? &blackPen : &grayPen, rect, aStart, aEnd - aStart);
		}
	}

	// Draw cross-hair
	graphics->DrawLine(&blackPen, toGdiPointF(util.centerPoint + Vector2::unitX * util.halfSize.x), toGdiPointF(util.centerPoint - Vector2::unitX * util.halfSize.x));
	graphics->DrawLine(&blackPen, toGdiPointF(util.centerPoint + Vector2::unitY * util.halfSize.y), toGdiPointF(util.centerPoint - Vector2::unitY * util.halfSize.y));

	// Create a font
	const WCHAR* goodFonts[4] = { L"Consolas", L"Lucida Console", L"Courier New", L"Arial" };
	Gdiplus::Font* font = nullptr;
	Gdiplus::Font* smallFont = nullptr;
	for (int i = 0; i < 4 && !font; i++)
	{
		font = new Gdiplus::Font(&Gdiplus::FontFamily(goodFonts[i]), 12);
		smallFont = new Gdiplus::Font(&Gdiplus::FontFamily(goodFonts[i]), 8);
		if (!font->IsAvailable() && !smallFont->IsAvailable()) { delete font; font = nullptr; delete smallFont; smallFont = nullptr; }
	}

	// Draw labels and tweak values
	if (font)
	{
		WCHAR buffer[1024];
		int writeAt = 0;
		for (auto t : util.tweakables) { if (t.label) writeAt += swprintf_s(buffer + writeAt, 1024 - writeAt, L"%s%s: %s: %7.2f\n\r", t.valueMultiplierAtSliderMax >= 1.0f ? L" " : L"-", t.bindingAxis ? L"y" : L"x", t.label, *t.variable); }
		swprintf_s(buffer + writeAt, 1024 - writeAt, L"    Num elements: %d", util.spline->displayShapes.size());

		Gdiplus::LinearGradientBrush brush(Gdiplus::Rect(0, 0, 100, 100), Gdiplus::Color::Gray, Gdiplus::Color::DimGray, Gdiplus::LinearGradientModeHorizontal);
		Vector2 textOrigin = util.centerPoint + Vector2::unitY * util.halfSize.y * 1.2f - Vector2::unitX * 165.0f;
		graphics->DrawString(buffer, -1, font, toGdiPointF(textOrigin), &brush);
		graphics->DrawString(L"less arcs", -1, smallFont, toGdiPointF(util.centerPoint + Vector2::unitX * util.halfSize.x * 1.f + Vector2(-55.0f, -12.0f)), &brush);
		graphics->DrawString(L"more arcs", -1, smallFont, toGdiPointF(util.centerPoint - Vector2::unitX * util.halfSize.x * 1.f + Vector2(0.0f, -12.0f)), &brush);
		graphics->DrawString(L"less segments", -1, smallFont, toGdiPointF(util.centerPoint - Vector2::unitY * util.halfSize.y * 1.f + Vector2(-40.0f, -15.0f)), &brush);
		graphics->DrawString(L"more segments", -1, smallFont, toGdiPointF(util.centerPoint + Vector2::unitY * util.halfSize.y * 1.f + Vector2(-40.0f, 0.0f)), &brush);
		graphics->DrawString(L"old value", -1, smallFont, toGdiPointF(util.initialPoint + Vector2(15.0f, 5.0f) ), &brush);
		delete font;
	}

	// Draw circular pointer
	Gdiplus::RectF pointRect(toGdiPointF(util.currPoint), Gdiplus::SizeF());
	for (int i = 0; i < 3; i++) { pointRect.Inflate(3.0f, 3.0f); graphics->DrawEllipse(&blackPen, pointRect); }

	// Draw original value marker
	pointRect = Gdiplus::RectF(toGdiPointF(util.initialPoint), Gdiplus::SizeF());
	for (int i = 0; i < 2; i++) { pointRect.Inflate(9.0f, 9.0f); graphics->DrawEllipse(&blackPen, pointRect); }
}

static std::pair<int, int> getCanvasSize(const HDC hdc)
{
	// From http://stackoverflow.com/questions/3154620/how-to-find-out-dcs-dimensions
	BITMAP structBitmapHeader;
	memset(&structBitmapHeader, 0, sizeof(BITMAP));

	HGDIOBJ hBitmap = GetCurrentObject(hdc, OBJ_BITMAP);
	GetObject(hBitmap, sizeof(BITMAP), &structBitmapHeader);

	return std::pair<int, int>(structBitmapHeader.bmWidth, structBitmapHeader.bmHeight);
}
