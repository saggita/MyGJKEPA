#pragma once

#include "CollisionObject.h"
#include "GJKSimplex.h"
#include "EPAEdge.h"
#include "EPATriangle.h"
#include "EPAPolytope.h"

class CNarrowCollisionInfo;

// Expanding Polytope Algorithm (EPA)
class CEPAAlgorithm
{
private:
	CEPAPolytope m_Polytope;
    int IsOriginInTetrahedron(const CVector3D& p1, const CVector3D& p2, const CVector3D& p3, const CVector3D& p4) const;
	
public:
    CEPAAlgorithm(); 
    ~CEPAAlgorithm();

    bool ComputePenetrationDepthAndContactPoints(const CGJKSimplex& simplex, CCollisionObject& objA, CCollisionObject& objB, CVector3D& v, CNarrowCollisionInfo* pCollisionInfo, int maxIteration = 30);
};