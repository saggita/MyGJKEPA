#ifndef CONVEX_HULL_UTILS_H
#define CONVEX_HULL_UTILS_H

#include <AdlPrimitives/Math/Math.h>
#include <AdlPrimitives/Math/Array.h>

using namespace adl;

class ConvexHullUtils
{
	public:
		static void createCvxShapeBF(const float4* points, int numPoints, Array<int4>& tris);
};

#endif

