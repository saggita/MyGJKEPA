#ifndef __MATH_H__
#define __MATH_H__

#define PI_F 3.1415926535898f
#define PI_D 3.1415926535897932384626433832795
#define EPSILON 1e-8
#define EPSILON1 1e-6

#include <cstdlib>
#include "Vector2D.h"

#define Sqr(a) ((a)*(a))
#define Cube(a) ((a)*(a)*(a))

inline bool IsZero(btScalar x) { return ( ( x < EPSILON ) && ( x > -EPSILON ) ? true : false ); }
inline bool IsEqual(btScalar a, btScalar b) { return ( ( ( a - b ) < EPSILON1 ) && ( ( a - b ) > -EPSILON1 ) ? true : false ); }

// linear interpolation dividing a, b with t : (1-t)
inline btScalar lerp(btScalar a, btScalar b, btScalar t)
{
	return a * (1.0f - t) + b * t;
}

inline CVector2D lerp(const CVector2D& a, const CVector2D& b, btScalar t)
{
	return a * (1.0f - t) + b * t;
}

// bilinear interpolation

/*

      v|             (i, j) ==> u, v
	   |           
	   |____u      
	   
	   Local coord
			      

           p    :  1-p
     c------------+----d
	 |            |    |
	 |            |    |
	 |       1-q  |    |
	 |            |    |
	 |            x    |
	 |            |    |
	 |         q  |    |
	 a------------+----b
           p    :  1-p

	x = bilerp(a, b, c, d, p, q)
	  = lerp(lerp(a, b, p), lerp(c, d, p), q)
*/

inline btScalar bilerp(btScalar a, btScalar b, btScalar c, btScalar d, btScalar p, btScalar q)
{
	return lerp(lerp(a, b, p), lerp(c, d, p), q);
}

inline CVector2D bilerp(const CVector2D& a, const CVector2D& b, const CVector2D& c, const CVector2D& d, btScalar p, btScalar q)
{
	return lerp(lerp(a, b, p), lerp(c, d, p), q);
}

/*

              v|             (i, j, k) ==> u, v, w
			   |           
			   |____u      
			   /Local coord
			 w/            

	                  c        p            1-p    d
			  		   ------------------+----------
					  /|                 |        /|
			  	     /                   |       / |
				    /  |                 |1-q   /  |
                   /                     |     /   |  
                  /    |                 |    /    |
			   g ------------------+---------- h   |
				 |     |           |     |   |     | 
				 |                 |     +   |     |
				 |     |           |   r/|   |     |
				 |                 |   / |q  |     |
				 |     |           |  x  |   |     |
		         |   a - - - - - - | / - + - |- - -| b
				 |    /            |/1-r     |     /
				 |                 +         |    / 
				 |  /              |         |   / 
				 |                 |         |  /
				 |/                |         | /
				 ------------------+----------
                e                            f


		x = trilerp(a, b, c, d, e, f, g, h, p, q, r)
		  = lerp(bilerp(a, b, c, d, p, q), bilerp(e, f, g, h, p, q), r)

*/
inline btScalar trilerp(btScalar a, btScalar b, btScalar c, btScalar d, btScalar e, btScalar f, btScalar g, btScalar h, btScalar p, btScalar q, btScalar r)
{
	return lerp(bilerp(a, b, c, d, p, q), bilerp(e, f, g, h, p, q), r);
}

inline CVector2D trilerp(const CVector2D& a, const CVector2D& b, const CVector2D& c, const CVector2D& d, 
						 const CVector2D& e, const CVector2D& f, const CVector2D& g, const CVector2D& h, 
						 btScalar p, btScalar q, btScalar r)
{
	return lerp(bilerp(a, b, c, d, p, q), bilerp(e, f, g, h, p, q), r);
}


/*
    (a + b)
	-------
	   2
*/
inline btScalar average2(btScalar a, btScalar b)
{
	return 0.5f * (a + b);
}

/*
    (a + b + c + d)
	---------------
	       4
*/
inline btScalar average4(btScalar a, btScalar b, btScalar c, btScalar d)
{
	return 0.25f * (a + b + c + d);
}

btScalar DistanceFromPointToTriangle(const CVector3D& point, const CVector3D& p0, const CVector3D& p1, const CVector3D& p2, CVector3D* closestPointInTriangle = NULL);

// If point is in the side of normal direction, returns positive closest distance. Otherwise, it returns negative closest distance.
// normal direction of triangle is (p1-p0).Cross(p2-p0).Normalize(). So p0, p1 and p2 forms counter-clockwise.
//btScalar SignedDistanceFromPointToTriangle(const CVector3D& point, const CVector3D& p0, const CVector3D& p1, const CVector3D& p2, CVector3D* closestPointInTriangle = NULL);

// The normal vector of plane formed by p0, p1 and p2 is (p1-p0).Cross(p2-p0).Normalize().
// If 'point' is on the positive side of plane, it returns positive distance. Otherwise, it returns negative distance. 
btScalar SignedDistanceFromPointToPlane(const CVector3D& point, const CVector3D& p0, const CVector3D& p1, const CVector3D& p2, CVector3D* closestPointInTriangle = NULL);

// Assumes planeEqn[0], planeEqn[1] and planeEqn[2] forms unit normal vector.
btScalar SignedDistanceFromPointToPlane(const CVector3D& point, const btScalar* planeEqn, CVector3D* closestPointInTriangle = NULL);

#endif // __MATH_H__