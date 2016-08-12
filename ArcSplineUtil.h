#pragma once

#include <vector>

struct Biarc;
class FreeformLine;

// Utility for constructing ArcSpline from a FreeformLine
//
// Functions here are ordered in which they're used by the algorithm.
//  - find corners
//  - identify segments between corners
//  - convert remaining line sections into biarcs
//    - by iteratively fitting biarcs into a line section, and finding a 'best' one
//    - and then continuing from the endpoint of the last created biarc
struct ArcSplineUtil
{
	// Input for determining which points on the line qualify as corners
	struct CornersInput
	{
		// Distance between consecutive points checked.
		float tStep = 1.0f;

		// Approximate minimum angle to qualify as a corner
		float innerMinAngleInDeg = 45.0f;

		// Maximum angle difference between two arbitrary points on each of corner's arms, to qualify the corner
		float outerMaxAngleInDeg = 25.0f;

		// Don't touch. Reduce number of false positives, by requiring several test positives in a row. Can't be big or we'll miss sharp corners.
		unsigned int minNumberTestPositivesInSeries = 2;

		// Don't touch. Reduce occurrence of duplicated corners by merging them if they're close together. Increase it if we spot double corners.
		unsigned int maxDistBetweenCornersToMerge = 4;

		// Don't touch. Distance factors for choosing angle-measurement points.
		float innerInterMeasurementFactor = 1.0f;
		float outerInterMeasurementFactor = 2.0f;
	};

	// Input for checking if a line section qualifies as a segment
	struct SegmentsInput
	{
		// Stepping distance when computing approximation error
		float tStep = 15.0f;

		// Maximum mean error to qualify a segment of reference length. This is multiplied by sqrt(length/refLenght) for when assessing error, to allow more error for longer segments. 
		float maxMeanErrorAtReferenceLength = 2.5f;

		// Don't touch. Reference length used to compute maxMeanError
		float referenceSegmentLength = 20.0f;
	};

	// Input for generating biarc-splines for line sections
	struct BiarcsInput
	{
		// Runtime data, ignore it here
		Range tBounds;

		// Processing step. We only look at the few points equally spaced along a line section.
		float tStep = 15.0f;

		// Maximum mean error allowed for any biarc
		float maxMeanError = 10.0f;

		// Don't touch. Specifies range and number of various DParam ratios to try when fitting a biarc to a line section. 
		float maxBiarcRatio = 5.0f;
		float minBiarcRatio = 1 / maxBiarcRatio; // choosing this gives subjective effect (you can just slip it to 1.0f), lower values seem to create more windy lines at times.
		int numBiarcRatioSamples = 9; // preferably uneven number

		// Balancing value used to discard longer biarcs if they have relatively larger error.
		// You can set it to 1.0f to always generate longest arcs withing allowed error, which may yield
		// a less-aesthetic spline.
		float distToErrorThreshold = 1.01f;

		// Disables distToErrorThreshold if biarc ends farther than endOfLineOkayFactor * tStep before the end of line section.
		// By default, used only when the biarc reaches the end of the line.
		float endOfLineOkayFactor = 0.0f; 

		// Eliminate the final short/unneded arc, by allowing degenerate biarcs at the end
		bool allowHalfArcAtSectionEnd = true;

		// Approximate tolerance for tangent error at the end of a section (when using a single arc as the last element)
		float endAngleTolerance = 15.0f;

		// Allow considerably larger angle tolerance, if a section can be approximated with a single arc.
		bool allowExtraToleranceForSingleArcSections = false;

		// Approximate tolerance for tangent error at the end of a section, when using a single arc to approximate the entire section.
		float endAngleToleranceForSingleArcSection = 45.0f;
	};

	// Combined input for all processing.
	struct ProcessingInput : RefCounted
	{
		CornersInput corners;
		SegmentsInput segments;
		BiarcsInput biarcs;
	};

	// Find corners.
	//
	// Corners are found as sections where tangent changes significantly, but stays relatively constant farther away in each direction.
	static void findCorners(const FreeformLine& line, const CornersInput& input, std::vector<Range>* result);

	// Check if a segment is a satisfactory approximation of a line section
	static bool isSegment(const FreeformLine& line, const Range segmentBounds, const SegmentsInput& input, float* outMeanError2);

	// Convert a line section into a series of biarcs.
	//
	// This function converts a FreeformLine section into a series of Biarcs. Starting at the
	// beginning it attempts to fit the longest & 'nicest' Biarc that fits within 
	// a maxMeanSquareError constraint. Biarcs can be uniquely defined given start/end
	// points & tangents and given the relative ratio of lengths of the two arcs
	// contained. Ratio of one gives balanced a arc. We actually iterate over different
	// ratio values to get better fit. To get a 'nice' Biarc the utility rejects biarcs 
	// where extra length causes relatively high error increase, as compared to earlier
	// results. 
	//
	// The final Biarc of a line can degenerate to a single arc, and if such solution is
	// found its error is modified to favor such solutions. Also error balancing is
	// turned off if a biarc can reach the end of the line. That limits occurrence of
	// oddly-looking short final arcs.
	static void convertLineToBiarcs(const FreeformLine& line, const BiarcsInput& input, std::vector<Biarc>* result);

	// Calculate error between the FreeformLine & a fitting shape.
	//
	// This is estimated by measuring distance between points along the FreeformLine section and the fittingShape.
	template <class TShape> static float calcMeanSquaredError(const FreeformLine& line, float tStart, float tStep, float tEnd, const TShape& fittingShape);

	// Calculate distance between biarc midpoint & the line. 
	//
	// This is a last-moment fix up for some incorrect biarc results.
	static float minDistToBiarcMidPoint(const FreeformLine& line, float tStart, float tStep, float tEnd, const Biarc& biarc);
};
