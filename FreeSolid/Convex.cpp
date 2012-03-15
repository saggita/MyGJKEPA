/*
FreeSOLID - Software Library for Interference Detection
Copyright (C) 1997-1998  Gino van den Bergen

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free
Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

Please send remarks, questions and bug reports to gino@win.tue.nl,
or write to:
Gino van den Bergen
Department of Mathematics and Computing Science
Eindhoven University of Technology
P.O. Box 513, 5600 MB Eindhoven, The Netherlands
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef USE_QHULL
extern "C"
{
#include "qhull_a.h"
}
#endif

#include <vector>
#include "Convex.h"
#include "BBox.h"
#include "Transform.h"

Scalar rel_error = 1e-6f; // relative error in the computed distance
Scalar abs_error = 1e-10f; // absolute error if the distance is almost zero
/*
Scalar rel_error = 1e-6; // relative error in the computed distance
Scalar abs_error = 1e-10; // absolute error if the distance is almost zero
*/

BBox Convex::bbox(const Transform& t) const 
{
	Point mini(t.getOrigin()[X] + 
		dot(t.getBasis()[X], support(-t.getBasis()[X])) - abs_error,
		t.getOrigin()[Y] + 
		dot(t.getBasis()[Y], support(-t.getBasis()[Y])) - abs_error,
		t.getOrigin()[Z] + 
		dot(t.getBasis()[Z], support(-t.getBasis()[Z])) - abs_error); 
	Point maxi(t.getOrigin()[X] + 
		dot(t.getBasis()[X], support(t.getBasis()[X])) + abs_error,
		t.getOrigin()[Y] + 
		dot(t.getBasis()[Y], support(t.getBasis()[Y])) + abs_error,
		t.getOrigin()[Z] + 
		dot(t.getBasis()[Z], support(t.getBasis()[Z])) + abs_error); 
	return BBox(mini,maxi);
}

static Point  p[4];   // support points of object A in local coordinates 
static Point  q[4];   // support points of object B in local coordinates 
static Vector y[4];   // support points of A - B in world coordinates

static std::vector<Vector> inflated_polyhedron; // Inflated Polyhedron for depth penetration

static int bits;      // identifies current simplex
static int last;      // identifies last found support point
static int last_bit;  // last_bit = 1<<last
static int all_bits;  // all_bits = bits|last_bit 

static Scalar det[16][4]; // cached sub-determinants

#ifdef STATISTICS
int num_iterations = 0;
int num_irregularities = 0;
#endif

void compute_det() 
{
	static Scalar dp[4][4];

	for (int i = 0, bit = 1; i < 4; ++i, bit <<=1) 
		if (bits & bit) dp[i][last] = dp[last][i] = dot(y[i], y[last]);
	dp[last][last] = dot(y[last], y[last]);

	det[last_bit][last] = 1;
	for (int j = 0, sj = 1; j < 4; ++j, sj <<= 1) 
	{
		if (bits & sj) {
			int s2 = sj|last_bit;
			det[s2][j] = dp[last][last] - dp[last][j]; 
			det[s2][last] = dp[j][j] - dp[j][last];
			for (int k = 0, sk = 1; k < j; ++k, sk <<= 1) {
				if (bits & sk) {
					int s3 = sk|s2;
					det[s3][k] = det[s2][j] * (dp[j][j] - dp[j][k]) + 
						det[s2][last] * (dp[last][j] - dp[last][k]);
					det[s3][j] = det[sk|last_bit][k] * (dp[k][k] - dp[k][j]) + 
						det[sk|last_bit][last] * (dp[last][k] - dp[last][j]);
					det[s3][last] = det[sk|sj][k] * (dp[k][k] - dp[k][last]) + 
						det[sk|sj][j] * (dp[j][k] - dp[j][last]);
				}
			}
		}
	}
	if (all_bits == 15) 
	{
		det[15][0] = det[14][1] * (dp[1][1] - dp[1][0]) + 
			det[14][2] * (dp[2][1] - dp[2][0]) + 
			det[14][3] * (dp[3][1] - dp[3][0]);
		det[15][1] = det[13][0] * (dp[0][0] - dp[0][1]) + 
			det[13][2] * (dp[2][0] - dp[2][1]) + 
			det[13][3] * (dp[3][0] - dp[3][1]);
		det[15][2] = det[11][0] * (dp[0][0] - dp[0][2]) + 
			det[11][1] * (dp[1][0] - dp[1][2]) +  
			det[11][3] * (dp[3][0] - dp[3][2]);
		det[15][3] = det[7 ][0] * (dp[0][0] - dp[0][3]) + 
			det[7 ][1] * (dp[1][0] - dp[1][3]) + 
			det[7 ][2] * (dp[2][0] - dp[2][3]);
	}
}

inline bool valid(int s) 
{  
	for (int i = 0, bit = 1; i < 4; ++i, bit <<= 1) 
	{
		if (all_bits & bit) 
		{
			if (s & bit) 
			{ 
				if (det[s][i] <= 0) return false; 
			}
			else if (det[s|bit][i] > 0) return false;
		}
	}
	return true;
}

inline void compute_vector(int bits1, Vector& v) 
{
	Scalar sum = 0;
	v.setValue(0, 0, 0);
	for (int i = 0, bit = 1; i < 4; ++i, bit <<= 1) 
	{
		if (bits1 & bit) {
			sum += det[bits1][i];
			v += y[i] * det[bits1][i];
		}
	}
	v *= 1 / sum;
}

inline void compute_points(int bits1, Point& p1, Point& p2) 
{
	Scalar sum = 0;
	p1.setValue(0, 0, 0);
	p2.setValue(0, 0, 0);
	for (int i = 0, bit = 1; i < 4; ++i, bit <<= 1) 
	{
		if (bits1 & bit) 
		{
			// if the points are the same they cancel each other
			sum += det[bits1][i];
			p1 += p[i] * det[bits1][i];
			p2 += q[i] * det[bits1][i];
		}
	}
	Scalar s = 1 / sum;
	p1 *= s;
	p2 *= s;
}

#ifdef USE_BACKUP_PROCEDURE

inline bool proper(int s) {  
	for (int i = 0, bit = 1; i < 4; ++i, bit <<= 1)
		if ((s & bit) && det[s][i] <= 0) return false; 
	return true;
}

#endif

//======================
// Johnson's Algorithm
//======================
inline bool closest(Vector& v) 
{
	compute_det();
	for (int s = bits; s; --s) 
	{
		if ((s & bits) == s) 
		{
			if (valid(s|last_bit)) 
			{
				bits = s|last_bit;
				compute_vector(bits, v);
				return true;
			}
		}
	}
	if (valid(last_bit)) 
	{
		bits = last_bit;
		v = y[last];
		return true;
	}
	// Original GJK calls the backup procedure at this point.

#ifdef USE_BACKUP_PROCEDURE

	Scalar min_dist2 = SOLID_INFINITY;
	for (int s = all_bits; s; --s) 
	{
		if ((s & all_bits) == s) 
		{
			if (proper(s)) 
			{
				Vector u;
				compute_vector(s, u);
				Scalar dist2 = u.length2();
				if (dist2 < min_dist2) {
					min_dist2 = dist2;
					bits = s;
					v = u;
				}
			}
		}
	}

#endif
	return false;
}

/*
The next function is used for detecting degenerate cases that cause 
termination problems due to rounding errors.
It checks for duplicate points
*/

inline bool degenerate(const Vector& w) 
{
	for (int i = 0, bit = 1; i < 4; ++i, bit <<= 1) 
	{
		if((all_bits & bit) && y[i] == w)  return true;
	}
	return false;
}


void closest_point_in_triangle(const Vector& p,
	const Vector& a,
	const Vector& b,
	const Vector& c, 
	Vector& out)
{
	// Check if P in vertex region outside A
	Vector ab = b - a;
	Vector ac = b - a;
	Vector ap = p - a;
	float d1 = dot(ab,ap);
	float d2 = dot(ac,ap);
	if (d1 <= 0.0f && d2 <= 0.0f)
	{ 
		out = a; // barycentric coordinates (1,0,0)
		//fprintf(stdout,"1\n");	
		return;
	}

	// Check if P in vertex region outside B
	Vector bp = p - b;
	float d3 = dot(ab,bp);
	float d4 = dot(ac,bp);
	if (d3 >= 0.0f && d4 <= d3)
	{
		out = b;
		return; // barycentric coordinates (0,1,0)
	}

	// Check if P in edge region of AB, if so return projection of P onto AB
	float vc = (d1*d4) - (d3*d2);
	if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) 
	{
		float v = d1 / (d1 - d3);
		out = a + (ab*v); // barycentric coordinates (1-v,v,0)
		return;
	}

	// Check if P in vertex region outside C
	Vector cp = p - b;
	float d5 = dot(ab,cp);
	float d6 = dot(ac,cp);
	if (d6 >= 0.0f && d5 <= d6) 
	{
		out=b;
		return; // barycentric coordinates (0,0,1)
	}

	// Check if P in edge region of AC, if so return projection of P onto AC
	float vb = (d5*d2) - (d1*d6);
	if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) 
	{
		float w = d2 / (d2 - d6);
		out = a + (ac*w); // barycentric coordinates (1-w,0,w)
		return;
	}

	// Check if P in edge region of BC, if so return projection of P onto BC
	float va = (d3*d6) - (d5*d4);
	if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) 
	{
		float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		out= b + ((b - b)*w); 
		// barycentric coordinates (0,1-w,w)
		return;
	}

	// P inside face region. Compute Q through its barycentric coordinates (u,v,w)
	float denom = 1.0f / (va + vb + vc);
	float v = vb * denom;
	float w = vc * denom;
	out = a + (ab * v) + (ac * w); 
}

void closest_point_in_line(const Vector& p, 
	const Vector& a, 
	const Vector& b,
	Vector& out)
{
	// Create the vector from end point a to our point p.
	Vector vVector1 = p - a;
	// Create a normalized direction vector from end point a to end point b
	Vector vVector2 = b - a;
	vVector2.normalize();
	// Use the distance formula to find the distance of 
	// the line segment (or magnitude)
	float d = sqrtf((vVector2[X]*vVector2[X])+
		(vVector2[Y]*vVector2[Y])+
		(vVector2[Z]*vVector2[Z]));

	// Using the dot product, we project the vVector1 onto the vector vVector2.
	// This essentially gives us the distance from our projected vector from a.
	float t = dot(vVector2,vVector1);
	// If our projected distance from a, "t", is less than or equal to 0,
	// it must be closest to the end point a.  We want to return this end point.
	if (t <= 0) 
	{
		out = a;
		return;
	}

	// If our projected distance from a, "t", 
	// is greater than or equal to the magnitude
	// or distance of the line segment, 
	// it must be closest to the end point b.  So, return b.
	if (t >= d)
	{ 
		out = b;
		return;
	}

	// Here we create a vector that is of length t 
	// and in the direction of vVector2
	Vector vVector3 = vVector2 * t;

	// To find the closest point on the line segment,
	// we just add vVector3 to the original
	// end point a.  
	Vector vClosestPoint = a + vVector3;
	// Return the closest point on the line segment
	out = vClosestPoint;
}

/*!
\brief Determines minimum penetration depth vector for current simplex
\param [IN] a Convex Shape A
\param [IN] b Convex Shape B
\param [IN] a2w A transformation
\param [IN] b2w B transformation
\param [OUT] v penetration depth vector
*/
inline void determine_minimum_penetration_depth(const Convex& a, 
	const Convex& b,
	const Transform& a2w, 
	const Transform& b2w,
	Vector& v)
{
#ifdef USE_QHULL
	/* 
	This is a very RAW implementation of the Penetration Depth calculation
	described on the paper 
	"Proximity Queries and Penetration Depth Computation on 3D Game Objects"
	found at http://www.win.tue.nl/~gino/solid/gdc2001depth.pdf
	*/
	boolT ismalloc;
	int curlong, totlong, exitcode;
	char options[200];
	vector<Vector> new_polyhedron;
	facetT *facet;
	vertexT *vertex;
	vertexT **vertexp;
	const Vector origin(0,0,0);
	Vector closest;
	Vector close_test;
	Vector w;
	Vector last;
	int triangle_index[3];
	Vector triangle[3];
	int iterations=0,vertcount;
	// LOOP Starts Here, testing with 100 iterations:
	//for(int j=0;j<100;++j)
	while(1)
	{
		size_t numVerts = inflated_polyhedron.size();

		coordT *array = new coordT[numVerts*3];
		coordT *p = &array[0];
		int i;
		for (i = 0; i < numVerts; ++i) 
		{
			*p++ = inflated_polyhedron[i][X];
			*p++ = inflated_polyhedron[i][Y];
			*p++ = inflated_polyhedron[i][Z];
		}

		ismalloc = False; // True if qh_freeqhull should 'free(array)'
		qh_init_A(stdin, stdout, stderr, 0, NULL);
		if (exitcode = setjmp(qh errexit)) exit(exitcode);
		sprintf(options, "qhull Qx i s Tcv C-0");
		qh_initflags(options);
		qh_init_B(&array[0], numVerts, 3, ismalloc);
		qh_qhull();
		qh_check_output();
		vertcount=0;

		Scalar distance=FLT_MAX;

		FORALLfacets 
		{      
			setT *vertices = qh_facet3vertex(facet);
			i=0;

			FOREACHvertex_(vertices) 
			{
				triangle_index[i]=qh_pointid(vertex->point);
				++i;
			}
			closest_point_in_triangle(origin,
				inflated_polyhedron[triangle_index[0]],
				inflated_polyhedron[triangle_index[1]],
				inflated_polyhedron[triangle_index[2]],
				close_test);
			if(close_test.length2()<distance)
			{
				triangle[0]=inflated_polyhedron[triangle_index[0]];
				triangle[1]=inflated_polyhedron[triangle_index[1]];
				triangle[2]=inflated_polyhedron[triangle_index[2]];
				distance=close_test.length2();
				closest=close_test;
			}      
		}  
		// Split triangle, add points to the convex hull
		FORALLvertices
		{
			vertcount++;
			new_polyhedron.push_back(inflated_polyhedron[qh_pointid(vertex->point)]);
		}
		inflated_polyhedron.swap(new_polyhedron);
		new_polyhedron.clear();
		w = a2w(a.support((-closest) * a2w.getBasis())) - 
			b2w(b.support(closest * b2w.getBasis()));
		if(w==last)
		{
			v=w;
			return;
		}
		last=w;
		inflated_polyhedron.push_back(w);

		for(i=0;i<3;++i)
		{
			closest_point_in_line(origin,triangle[i],triangle[(i+1)%3],closest);
			w = a2w(a.support((-closest) * a2w.getBasis())) - 
				b2w(b.support(closest * b2w.getBasis())); 
			if(dot(w,closest)!=dot(closest,closest))
			{
				inflated_polyhedron.push_back(w);
			}
		}  
		delete [] array;  
		qh NOerrexit = True;
		qh_freeqhull(!qh_ALL);
		qh_memfreeshort(&curlong, &totlong);
		iterations++;
	}
#else
	//#warning "Determination of minimum penetration depth without QHULL has not been implemented"
#endif
}

/*! 
\brief Checks whether or not convex shapes A and B intersect
\param a   [IN]  Convex Shape A
\param b   [IN]  Convex Shape B
\param a2w [IN]  Transformation Matrix for A
\param b2w [IN]  Transformation Matrix for B
\param v   [OUT] Separating axis
\note This is the core of the GJK Algorithm
*/
bool intersect(const Convex& a, 
	const Convex& b,
	const Transform& a2w, 
	const Transform& b2w,
	Vector& v) 
{

	//==========
	//	GJK
	//==========

	inflated_polyhedron.clear();
	Vector w;

	bits = 0;
	all_bits = 0;

#ifdef STATISTICS
	num_iterations = 0;
#endif
	do 
	{
		last = 0;
		last_bit = 1;

		while (bits & last_bit) 
		{ 
			++last; 
			last_bit <<= 1; 
		}

		w = a2w(a.support((-v) * a2w.getBasis())) - 
			b2w(b.support(v * b2w.getBasis())); 

		if (dot(v, w) > 0) 
			return false;

		if (degenerate(w)) 
		{
#ifdef STATISTICS
			++num_irregularities;
#endif
			return false;
		}

		y[last] = w;
		all_bits = bits|last_bit;
#ifdef STATISTICS
		++num_iterations;
#endif

		if (!closest(v)) 
		{
#ifdef STATISTICS
			++num_irregularities;
#endif
			return false;
		}
	}
	while (bits < 15 && !approxZero(v));


	//====================
	// EPA begins here.
	//====================

	switch(last)
	{
	case 0:
		/* 
		Simplex is a point this happens if the 2 surfaces barelly touch
		*/
		v[X]=0.0f;
		v[Y]=0.0f;
		v[Z]=0.0f;
		break;
	case 1:
		// The Simplex is line, return the point closer to the origin
		if(y[0].length2()<y[1].length2())
		{
			v=y[0];
		}
		else
		{
			v=y[1];	  
		}
		break;
	case 2:
		// The simplex is a triangle, add support points in the normal
		// of the triangle direction and the inverse of the normal
		v.normal(y[0],y[1],y[2]);
		w = a2w(a.support((-v) * a2w.getBasis())) - 
			b2w(b.support(v * b2w.getBasis())); 
		inflated_polyhedron.push_back(w);
		v*=-1;
		w = a2w(a.support((-v) * a2w.getBasis())) - 
			b2w(b.support(v * b2w.getBasis())); 
		inflated_polyhedron.push_back(w);
		// no break here since we go thru the same process as if this was
		// a tetrahedron.
	case 3:
		// The simplex is a tetrahedron, just what we need!
		for(int i=0;i<=last;++i)
		{
			inflated_polyhedron.push_back(y[i]);
		}
		determine_minimum_penetration_depth(a,b,a2w,b2w,v);
		break;
	}
	return true;
}

bool intersect(const Convex& a, const Convex& b, const Transform& b2a, 
	Vector& v) {
		Vector w;

		bits = 0;
		all_bits = 0;

#ifdef STATISTICS
		num_iterations = 0;
#endif

		do {
			last = 0;
			last_bit = 1;
			while (bits & last_bit) { ++last; last_bit <<= 1; }
			w = a.support(-v) - b2a(b.support(v * b2a.getBasis()));
			if (dot(v, w) > 0) return false;
			if (degenerate(w)) {
#ifdef STATISTICS
				++num_irregularities;
#endif
				return false;
			}
			y[last] = w;
			all_bits = bits|last_bit;
#ifdef STATISTICS
			++num_iterations;
#endif
			if (!closest(v)) {
#ifdef STATISTICS
				++num_irregularities;
#endif
				return false;
			}
		} 
		while (bits < 15 && !approxZero(v)); 
		return true;
}

bool common_point(const Convex& a, const Convex& b,
	const Transform& a2w, const Transform& b2w,
	Vector& v, Point& pa, Point& pb) {
		Vector w;

		bits = 0;
		all_bits = 0;

#ifdef STATISTICS
		num_iterations = 0;
#endif

		do {
			last = 0;
			last_bit = 1;
			while (bits & last_bit) { ++last; last_bit <<= 1; }
			p[last] = a.support((-v) * a2w.getBasis());
			q[last] = b.support(v * b2w.getBasis());
			w = a2w(p[last]) - b2w(q[last]);
			if (dot(v, w) > 0) return false;
			if (degenerate(w)) {
#ifdef STATISTICS
				++num_irregularities;
#endif
				return false;
			}
			y[last] = w;
			all_bits = bits|last_bit;
#ifdef STATISTICS
			++num_iterations;
#endif
			if (!closest(v)) {
#ifdef STATISTICS
				++num_irregularities;
#endif
				return false;
			}
		}
		while (bits < 15 && !approxZero(v) ) ;
		compute_points(bits, pa, pb);
		return true;
}

bool common_point(const Convex& a, const Convex& b, const Transform& b2a,
	Vector& v, Point& pa, Point& pb) {
		Vector w;

		bits = 0;
		all_bits = 0;

#ifdef STATISTICS
		num_iterations = 0;
#endif

		do {
			last = 0;
			last_bit = 1;
			while (bits & last_bit) { ++last; last_bit <<= 1; }
			p[last] = a.support(-v);
			q[last] = b.support(v * b2a.getBasis());
			w = p[last] - b2a(q[last]);
			if (dot(v, w) > 0) return false;
			if (degenerate(w)) {
#ifdef STATISTICS
				++num_irregularities;
#endif
				return false;
			}
			y[last] = w;
			all_bits = bits|last_bit;
#ifdef STATISTICS
			++num_iterations;
#endif
			if (!closest(v)) {
#ifdef STATISTICS
				++num_irregularities;
#endif
				return false;
			}
		}
		while (bits < 15 && !approxZero(v) );   
		compute_points(bits, pa, pb);
		return true;
}

#ifdef STATISTICS
void catch_me() {}
#endif

/*! \brief Gets the closest points to each of the shapes ?
*/
void closest_points(const Convex& a, const Convex& b,
	const Transform& a2w, const Transform& b2w,
	Point& pa, Point& pb) 
{
	static Vector zero(0, 0, 0);
	// Transform a's support point relative to the origin as well
	// as b's, substract b's transformed support point from a's
	Vector v = a2w(a.support(zero)) - b2w(b.support(zero));
	// Find the distance from the origin to the point found.
	Scalar dist = v.length();

	Vector w;

	bits = 0;
	all_bits = 0;
	Scalar mu = 0;

#ifdef STATISTICS
	num_iterations = 0;
#endif

	while (bits < 15 && dist > abs_error) {
		last = 0;
		last_bit = 1;
		while (bits & last_bit) { ++last; last_bit <<= 1; }
		p[last] = a.support((-v) * a2w.getBasis());
		q[last] = b.support(v * b2w.getBasis());
		w = a2w(p[last]) - b2w(q[last]);
		set_max(mu, dot(v, w) / dist);
		if (dist - mu <= dist * rel_error) break; 
		if (degenerate(w)) {
#ifdef STATISTICS
			++num_irregularities;
#endif
			break;
		}
		y[last] = w;
		all_bits = bits|last_bit;
#ifdef STATISTICS
		++num_iterations;
		if (num_iterations > 1000) catch_me();
#endif
		if (!closest(v)) {
#ifdef STATISTICS
			++num_irregularities;
#endif
			break;
		}
		dist = v.length();
	}
	compute_points(bits, pa, pb);
}

