#include "FreeformTool.h"
#include "TweakUtil.h"

#include "ArcSpline.h"

void TweakUtil::attach(ArcSpline* spline, const Vector2& guiAnchorPoint)
{
	ME_ASSERT(spline);

	this->spline = spline;
	centerPoint = guiAnchorPoint;
	initialPoint = centerPoint;

	currPoint = guiAnchorPoint;
	halfSize = Vector2(200.0f, 200.0f);
	wasUpdated = false;

	ArcSplineUtil::ProcessingInput referenceInput;

	// Values to use in final: spline error 1-10-100
	//          angles (x2) 22-45-90 (+), sync other segment values at same rate

	float rangeX = 4.0f;
	float rangeY = 1.0f / 3.0f;

	// Calc where current value sits on the tweak chart
	// value = orgValue * pow(multiplier, diffOnAxis)
	// log_multiplier(value/orgValue) = diffOnAxis = log(value/orgValue) / log(multiplier)
	float startX = std::logf(spline->processingInput->biarcs.maxMeanError / referenceInput.biarcs.maxMeanError) / std::logf(rangeX);
	float startY = std::logf(spline->processingInput->segments.maxMeanErrorAtReferenceLength / referenceInput.segments.maxMeanErrorAtReferenceLength) / std::logf(rangeY);
	centerPoint -= Vector2(startX, -startY).scale(halfSize);

	tweakables.push_back({ L"Max Mean Spline Error  ", &spline->processingInput->biarcs.maxMeanError, referenceInput.biarcs.maxMeanError, rangeX, 0 });
	tweakables.push_back({ L"Max Mean Segment Error ", &spline->processingInput->segments.maxMeanErrorAtReferenceLength, referenceInput.segments.maxMeanErrorAtReferenceLength, rangeY, 1 });

}

void TweakUtil::detach()
{
	spline = nullptr;
	wasUpdated = false;
	tweakables.clear();
}

void TweakUtil::update(const Vector2& guiMousePoint)
{
	ME_ASSERT(isAttached());
	wasUpdated = true;

	Vector2 diff = guiMousePoint - centerPoint;
	diff.x = getClipped(diff.x / halfSize.x, -1.0f, 1.0f);
	diff.y = getClipped(diff.y / halfSize.y, -1.0f, 1.0f);

	currPoint = centerPoint + diff.scale(halfSize);

	diff.y *= -1.0f; // Reverse vertical diff.

	for (auto& t : tweakables) { *t.variable = t.referenceValue * std::pow(t.valueMultiplierAtSliderMax, diff[t.bindingAxis]); }

	// hand code, allowing extra error for single arcs:
	spline->processingInput->biarcs.allowExtraToleranceForSingleArcSections = (0.98f < diff.x);

	spline->recreateSpline();
}
