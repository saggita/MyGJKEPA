#pragma once

#include "ConvexCollisionAlgorithm.h"
#include "EPAPolytope.h"

class CCollisionObject;
class CEPAEdge;
class CGJKSimplex;
class CWorldSimulation;
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