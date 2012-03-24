#ifndef CUSTOM_CONVEX_SHAPE_H
#define CUSTOM_CONVEX_SHAPE_H

include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "ConvexHeightFieldShape.h"

class CustomConvexShape : public btConvexHullShape
{
	public:
		
		ConvexHeightField* m_ConvexHeightField;

		CustomConvexShape(const btScalar* points,int numPoints,int stride);
		virtual ~CustomConvexShape();
		
};

#endif //CUSTOM_CONVEX_SHAPE_H

