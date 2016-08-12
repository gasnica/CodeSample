#include "FreeformTool.h"

#include <fstream>
#include <iostream>
#include <vector>

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <windowsx.h>
#include <objidl.h>
#include <gdiplus.h>

#include "ArcSpline.h"
#include "Common.h"
#include "FreeformLine.h"
#include "ShapeDrawer.h"
#include "TweakUtil.h"
#include "Vector2.h"

/*
 Started from this example: https://msdn.microsoft.com/en-us/library/windows/desktop/ms533895(v=vs.85).aspx
*/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This contains the WinMain function & all windows functionality, apart from
drawing being also performed in ShapeDrawer.

Use mouse + LMB for drawing on the app canvas. You can press C/S/L for clearing,
saving (and overwriting), and loading the lines. Only input lines are saved.
ArcSplines are recomputed on load. A single file "lines.dat" is used for storing
data.

See: ArcSpline, FreeformLine,  ShapeDrawer
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static ref<FreeformLine> g_activeLine = nullptr;
static ref<ArcSpline> g_selectedSpline = nullptr;
static std::vector<ref<ArcSpline>> g_arcSplines;

static TweakUtil g_tweakUtil;
bool g_forceDrawAll = false;

const char g_saveFileName[] = "lines.dat";

using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

VOID OnPaint(HDC hdc)
{
	static int nextPartialDrawStart = 0;
	if (g_activeLine && !g_forceDrawAll)
	{
		ShapeDrawer drawer(hdc, ShapeDrawer::MODE_FAST_AND_PARTIAL);
		nextPartialDrawStart = drawer.drawFreeformLine(*g_activeLine, nextPartialDrawStart);
	}
	else
	{
		ShapeDrawer drawer(hdc, ShapeDrawer::MODE_SLOW_BUT_DOUBLEBUFFERED);
		drawer.clear(Gdiplus::Color::White);

		g_forceDrawAll = false;
		nextPartialDrawStart = 0;

		for (const ArcSpline* spline : g_arcSplines) { drawer.drawFreeformLine(*spline->sourceLine); }
		for (const ArcSpline* spline : g_arcSplines) { drawer.drawArcSpline(*spline, g_selectedSpline == spline ? 3.5f : 2.0f); }

		if (g_tweakUtil.isActive()) { drawer.drawTweakUtil(g_tweakUtil); }
	}
}

// Clear all lines & cancel drawing
void globalClear()
{
	g_activeLine = nullptr;
	g_arcSplines.clear();
}

// Clear all, load FreeformLines from file, regenerate ArcSplines with default parameters.
void globalLoad()
{
	globalClear();

	std::filebuf fb;
	if (fb.open(g_saveFileName, std::ios::in))
	{
		std::istream is(&fb);
		int numLines;
		is >> numLines;
		for (int i = 0; i < numLines; i++)
		{
			ref<FreeformLine> line = new FreeformLine();
			is >> *line;
			g_arcSplines.push_back(new ArcSpline(line));
		}
		fb.close();
	}
}

// Save all created FreeformLines to a file. Don't save ArcSplines, or their modified parametes.
void globalSave()
{
	std::filebuf fb;
	if (fb.open(g_saveFileName, std::ios::out))
	{
		std::ostream os(&fb);
		os << g_arcSplines.size() << " ";
		for (const ref<ArcSpline>& s : g_arcSplines) { os << *s->sourceLine; }
		fb.close();
	}
}

// Find the latest ArcSpline within a distance from a point. Also note if we're hitting an endpoint of an element.
ArcSpline* globalFindLatestElementInDistance(const Vector2& point, bool* outIsEndpointHit, float maxDist = 5.0f, float testDistForEndpoints = 5.0f)
{
	// process splines and elements starting at the latest, for intuitive selection
	*outIsEndpointHit = false;
	for (auto it = g_arcSplines.rbegin(); it != g_arcSplines.rend(); ++it)
	{
		std::vector<ref<SplineElement>>& elements = (*it)->displayShapes;
		
		for (auto elemIt = elements.rbegin(); elemIt != elements.rend(); ++elemIt)
		{
			const SplineElement& elem = **elemIt;
			float dist = elem.distTo(point);
			if (dist <= maxDist)
			{
				*outIsEndpointHit = elem.distToEndPoint(point) <= testDistForEndpoints;
				return *it;
			}
		}
	}
	return nullptr;
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, INT iCmdShow)
{
#if defined _DEBUG
	FPExceptionEnabler exceptionEnabler;
#endif

	HWND                hWnd;
	MSG                 msg;
	WNDCLASS            wndClass;
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;

	// Initialize GDI+.
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = TEXT("GettingStarted");

	RegisterClass(&wndClass);

	hWnd = CreateWindow(
		TEXT("GettingStarted"),   // window class name
		TEXT("Drawing Prototype"),  // window caption
		WS_OVERLAPPEDWINDOW,      // window style
		CW_USEDEFAULT,            // initial x position
		CW_USEDEFAULT,            // initial y position
		CW_USEDEFAULT,            // initial x size
		CW_USEDEFAULT,            // initial y size
		NULL,                     // parent window handle
		NULL,                     // window menu handle
		hInstance,                // program instance handle
		NULL);                    // creation parameters

	ShowWindow(hWnd, iCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	ME_ON_DEBUG(globalClear());
	GdiplusShutdown(gdiplusToken);
	return msg.wParam;
}  // WinMain

LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
	WPARAM wParam, LPARAM lParam)
{
	HDC          hdc;
	PAINTSTRUCT  ps;

	switch (message)
	{
	case WM_MOUSEMOVE:
		if (g_activeLine)
		{
			// Append point to line
			g_activeLine->addPoint(Vector2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));

			// When drawing starts, unselect the hightlighted spline, and redraw all
			if (g_selectedSpline)
			{
				g_selectedSpline = nullptr;
				g_forceDrawAll = true; 
			}

			InvalidateRect(hWnd, NULL, false);
		}
		if (g_tweakUtil.isAttached())
		{
			g_tweakUtil.update(Vector2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
			InvalidateRect(hWnd, NULL, false);
		}
		return 0;
	case WM_LBUTTONDOWN:
		if (!g_activeLine)
		{
			Vector2 clickPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			// Check if we're clicking on an existing spline element
			bool isEndpoint;
			ArcSpline* found = globalFindLatestElementInDistance(clickPoint, &isEndpoint);
			if (g_selectedSpline != found) 
			{
				// Redraw all, when hightlighting/selecting a new arcSpline
				g_forceDrawAll = true; 
				InvalidateRect(hWnd, NULL, false); 
			}
			g_selectedSpline = found;

			if (g_selectedSpline && !isEndpoint)
			{
				// Edit the found shape
				g_tweakUtil.attach(g_selectedSpline, clickPoint);
			}
			else
			{
				// Allow visual selection of g_selectedSpline, and also prepare to 
				//
				// Start drawing a new shape
				g_activeLine = new FreeformLine();
				g_activeLine->addPoint(clickPoint);
			}
		}
		return 0;
	case WM_LBUTTONUP:
		if (g_activeLine && 0.0f < g_activeLine->length()) 
		{
			// Create a new ArcSpline
			g_arcSplines.push_back(new ArcSpline(g_activeLine)); 
		}
		g_activeLine = nullptr;
		g_tweakUtil.detach();
		InvalidateRect(hWnd, NULL, false);
		return 0;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'C': globalClear(); break;
		case 'L': globalLoad(); break;
		case 'S': globalSave(); break;
		}
		InvalidateRect(hWnd, NULL, false);
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		OnPaint(hdc);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
} // WndProc

