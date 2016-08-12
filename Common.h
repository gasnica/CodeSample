#pragma once

#include <cassert>
#include <cfloat>
#include <cmath>

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This is a set of generic constants, types, and functions, and also a couple of
ArcSpline settings.

ref & RefCounted are an intrusive smart pointer & base RefCounted
implementation.

FPExceptionEnabler enables floating-point exceptions in its scope to find
potential performance hits.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if _DEBUG
#	define ME_ON_DEBUG(code) (code)
#	define ME_ASSERT(condition) assert(condition)
#else
#	define ME_ON_DEBUG(code)
#	define ME_ASSERT(condition)
//#	define ME_ASSERT(condition) if (!(condition)) *(int*)nullptr = 0;
#endif

#include "Vector2.h" // include after ME_ASSERT is defined


#define ME_EPSILON 1e-6f
#define ME_EPSILON2 (ME_EPSILON * ME_EPSILON)
#define ME_PI 3.14159265358979323846264338328f
#define ME_DEG_TO_RAD (ME_PI / 180.0f)
#define ME_RAD_TO_DEG (180.0f / ME_PI)
#define ME_A_LOT 1e10f // Used instead of FLT_MAX to avoid float overflows when dividing by small numbers
#define ME_MUST_BE_ZERO 0 // Used to initialize 'invalid' value of enumerations

// ArcSplnie generation algorithm settings
#define ME_MAX_SPLINE_GAP 1.0f
#define ME_MAX_D_PARAM 10000.0f
#define ME_MAX_ARC_RADIUS 10000.0f
#define ME_MAX_ARC_RADIUS_TO_CHORD_LENGTH_RATIO 1000.0f // used for numerical stability when fitting circle


// Return value clipped to a range defined by min & max
inline float getClipped(float val, float min, float max) { ME_ASSERT(min <= max + ME_EPSILON); return std::fmin(std::fmax(min, val), max); }

// Are two values within 'precision' distance of each other.
inline bool isEqual(float a, float b, float precision = ME_EPSILON) { return std::fabs(a - b) <= precision; }


// Defines a float range
struct Range
{
	// Initialize as invalid
	Range() { invalidate(); }

	// Initialize with valid start/end values
	Range(float s, float e) : start(s), end(e) { ME_ASSERT(isValid()); }

	// Length of the Range; 0.0f if invalid
	float length() const { return isValid() ? end - start : 0.0f; }

	// Is this a valid/initialized range
	bool isValid() const { return start <= end; }

	// Reset the range & mark it invalid
	void invalidate() { start = FLT_MAX; end = -FLT_MAX; }

	// Expand the range to include the new value
	void include(float value) { start = std::fmin(value, start); end = std::fmax(end, value); }

	// Expand the range by padding on both sides
	void inflate(float padding) { ME_ASSERT(padding >= 0.0f); start -= padding; end += padding; } // doesn't check for shrinking

	// Start & end values of the range
	float start, end;

	// Comparison function for sorting ranges
	static bool isLess(const Range& a, const Range& b) { return a.start < b.start || (a.start == b.start && a.end < b.end); }
};


// An intrusive smart pointer; it implicitly casts to/from naked pointers.
//
// Referenced object should derive from RefCounted.
template <class T> class ref
{
public:
	// Construct empty, from a naked pointer, copy, and destruct
	ref() : ptr(nullptr) { }
	ref(T* ptr) : ptr(ptr) { if (ptr) ptr->addRef(); }
	ref(const ref<T>& other) : ref(other.ptr) { }
	~ref() { if (ptr) ptr->removeRef(); }

	// Assign from a naked or smart pointer
	const ref& operator = (T* other) { if (other) other->addRef(); if (ptr) ptr->removeRef(); ptr = other; return *this; } // Handles assignment to self
	const ref& operator = (const ref<T>& other) { return operator = (other.ptr); }

	// Treat a ref<T> like a normal pointer; implicitly cast to it & access members of pointed object
	operator T* () const { return ptr; }
	T* operator -> () const  { return ptr; }

private:
	// Referenced object; derive it from RefCounted
	T* ptr;
};


// Base class for objects referenced with the ref<class T> intrusive pointer.
class RefCounted
{
public:
	// Grant access to addRef/removeRef for ref<class T> 
	template <class T> friend class ref;

protected:
	RefCounted() : refCount(0) { }
	virtual ~RefCounted() { ME_ASSERT(refCount == 0); }

	// Prevent copying of refCount between objects in construction, assignment, and swapping
	RefCounted(const RefCounted&) : RefCounted() { }
	RefCounted& operator = (const RefCounted&) { return *this; }
	void swap(RefCounted&) { }

private:
	// Add reference
	void addRef() const { ++refCount; }

	// Decrease reference count & delete itself if the count falls to zero.
	void removeRef() const { if (0 == --refCount) delete this; }

	// Allow refCount update for const-type refs
	mutable int refCount; 
};


// Declare an object of this type in a scope in order to enable a
// specified set of floating-point exceptions temporarily. The old
// exception state will be reset at the end.
// This class can be nested.
// From https://randomascii.wordpress.com/2012/04/21/exceptional-floating-point/
class FPExceptionEnabler
{
public:
	// Overflow, divide-by-zero, and invalid-operation are the FP
	// exceptions most frequently associated with bugs.
	FPExceptionEnabler(unsigned int enableBits = _EM_OVERFLOW |
		_EM_ZERODIVIDE | _EM_INVALID | _EM_UNDERFLOW | _EM_DENORMAL ) // _EM_INEXACT triggers in Gdiplus
	{
		// Retrieve the current state of the exception flags. This
		// must be done before changing them. _MCW_EM is a bit
		// mask representing all available exception masks.
		_controlfp_s(&mOldValues, 0, 0);

		// Make sure no non-exception flags have been specified,
		// to avoid accidental changing of rounding modes, etc.
		enableBits &= _MCW_EM;

		// Clear any pending FP exceptions. This must be done
		// prior to enabling FP exceptions since otherwise there
		// may be a ‘deferred crash’ as soon the exceptions are
		// enabled.
		_clearfp();

		// Zero out the specified bits, leaving other bits alone.
		_controlfp_s(0, ~enableBits, enableBits);
		// After failing to undo, run this instead of the line above
		//_controlfp_s(0, ~enableBits, _MCW_EM);
	}
	~FPExceptionEnabler()
	{
		// Reset the exception state.
		_controlfp_s(0, mOldValues, _MCW_EM);
	}

private:
	unsigned int mOldValues;

	// Make the copy constructor and assignment operator private
	// and unimplemented to prohibit copying.
	FPExceptionEnabler(const FPExceptionEnabler&);
	FPExceptionEnabler& operator=(const FPExceptionEnabler&);
};