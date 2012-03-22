#pragma once

#include "Vector3D.h"

double DistancePointToEdge(const CVector3D& p, const CVector3D& x0, const CVector3D& x1, double& t, CVector3D& n);

// x00 and x01 are two vertices of edge0 in cloth0
// x10 and x11 are two vertices of edge1 in cloth1
// The closest point on edge0 is (p)*x00 + (1-p)*x01
// The closest point on edge1 is (q)*x10 + (1-q)*x11
// n is a normalized vector pointing from edge0 towards edge1
double DistanceEdgeToEdge(const CVector3D& x00, const CVector3D& x01, const CVector3D& x10, const CVector3D& x11, double& p, double& q, CVector3D& n, bool bClampToNearest = false);

// p is a point in the space not coinciding on triangle whose vertices are vert0, vert1 and vert2
// the closest point in triangle is a*x0 + b*x1 + c*x2. a, b and c are barycentric coordinates
// n is a normalized vector pointing from triangle towards the point
double DistancePointToTriangle(const CVector3D& p, const CVector3D& x0, const CVector3D& x1, const CVector3D& x2,double& a, double& b, double& c, CVector3D& n, bool bClampToNearest = false);

// x00 and x01 are two vertices of edge0 in cloth0
// x10 and x11 are two vertices of edge1 in cloth1
// xXX_new is a new position of xXX after time-step. e.g. x01_new is a new position of x01_old.
// The colliding point on edge0 is (p)*x00_old + (1-p)*x01_old
// The colliding point on edge1 is (q)*x10_old + (1-q)*x11_old
// n is a normalized vector pointing from edge0 towards edge1
bool ContinuousCollisionEdgeAndEdge(const CVector3D& x00_old, const CVector3D& x01_old, const CVector3D& x10_old, const CVector3D& x11_old,
	                      const CVector3D& x00_new, const CVector3D& x01_new, const CVector3D& x10_new, const CVector3D& x11_new, 
						  double& p, double& q, CVector3D& n, double& time);

// p is a point in the space not coinciding on triangle whose vertices are x0, x1 and x2
// the colliding point in triangle is a*x0_old + b*x1_old + c*x2_old. a, b and c are barycentric coordinates
// n is a normalized vector pointing from triangle towards the point
bool ContinuousCollisionPointToTriangle(const CVector3D& p_old, const CVector3D& x0_old, const CVector3D& x1_old, const CVector3D& x2_old,
				                const CVector3D& p_new, const CVector3D& x0_new, const CVector3D& x1_new, const CVector3D& x2_new,
								double& a, double& b, double& c, CVector3D& n, double& time);

