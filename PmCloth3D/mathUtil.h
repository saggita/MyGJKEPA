#ifndef __MATH_H__
#define __MATH_H__

#define PI_F 3.1415926535898f
#define PI_D 3.1415926535897932384626433832795
#define EPSILON 1e-8
#define EPSILON1 1e-6

#include "Vector2D.h"

#define Sqr(a) ((a)*(a))
#define Cube(a) ((a)*(a)*(a))

inline bool IsZero(double x) { return ( ( x < EPSILON ) && ( x > -EPSILON ) ? true : false ); }
inline bool IsEqual(double a, double b) { return ( ( ( a - b ) < EPSILON1 ) && ( ( a - b ) > -EPSILON1 ) ? true : false ); }

// linear interpolation dividing a, b with t : (1-t)
inline double lerp(double a, double b, double t)
{
	return a * (1.0 - t) + b * t;
}

inline CVector2D lerp(const CVector2D& a, const CVector2D& b, double t)
{
	return a * (1.0 - t) + b * t;
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

inline double bilerp(double a, double b, double c, double d, double p, double q)
{
	return lerp(lerp(a, b, p), lerp(c, d, p), q);
}

inline CVector2D bilerp(const CVector2D& a, const CVector2D& b, const CVector2D& c, const CVector2D& d, double p, double q)
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
inline double trilerp(double a, double b, double c, double d, double e, double f, double g, double h, double p, double q, double r)
{
	return lerp(bilerp(a, b, c, d, p, q), bilerp(e, f, g, h, p, q), r);
}

inline CVector2D trilerp(const CVector2D& a, const CVector2D& b, const CVector2D& c, const CVector2D& d, 
						 const CVector2D& e, const CVector2D& f, const CVector2D& g, const CVector2D& h, 
						 double p, double q, double r)
{
	return lerp(bilerp(a, b, c, d, p, q), bilerp(e, f, g, h, p, q), r);
}


/*
    (a + b)
	-------
	   2
*/
inline double average2(double a, double b)
{
	return 0.5 * (a + b);
}

/*
    (a + b + c + d)
	---------------
	       4
*/
inline double average4(double a, double b, double c, double d)
{
	return 0.25 * (a + b + c + d);
}

#endif // __MATH_H__