#pragma once

#include "CollisionObject.h"
#include "Simplex.h"
#include "EPAEdge.h"
#include "NarrowPhaseGJK.h"
#include "EPATriangle.h"
#include "EPAPolytope.h"



// Class TriangleComparison that allow the comparison of two triangles in the heap
// The comparison between two triangles is made using their square distance to the closest
// point to the origin. The goal is that in the heap, the first triangle is the one with the
// smallest square distance.
class TriangleComparison {
    public:
        // Comparison operator
        bool operator()(const CEPATriangle* face1, const CEPATriangle* face2) {
            return (face1->GetDistSqr() > face2->GetDistSqr());
        }
};

// Expanding Polytope Algorithm (EPA)
//==================
class CEPAAlgorithm
//==================
{
private:
    TriangleComparison triangleComparison;           // Triangle comparison operator
	CEPAPolytope m_Polytope;

    void AddFaceCandidate(CEPATriangle* triangle, CEPATriangle** heap, unsigned int& nbTriangles, double upperBoundSquarePenDepth);      // Add a triangle face in the candidate triangle heap
    int IsOriginInTetrahedron(const CVector3D& p1, const CVector3D& p2, const CVector3D& p3, const CVector3D& p4) const;        // Decide if the origin is in the tetrahedron

public:
    CEPAAlgorithm(); 
    ~CEPAAlgorithm();

    bool ComputePenetrationDepthAndContactPoints(const CGJKSimplex& simplex, const CCollisionObject& objA, const CCollisionObject& objB, CVector3D& v);
};