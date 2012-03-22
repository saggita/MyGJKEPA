#include <algorithm>
#include <limits>

#include "CollisionDetections.h"
#include "mathUtil.h"


inline CVector3D norm(const CVector3D& a, const CVector3D& b, const CVector3D& c, const CVector3D& d)
{
	return (a-b).Cross(c-d);
}

inline CVector3D norm(const CVector3D& p1, const CVector3D& p2, const CVector3D& p3)
{
	return (p2-p1).Cross(p3-p1);
}

inline double Triple(const CVector3D& a, const CVector3D& b, const CVector3D& c)
{ 
	return a.Dot(b.Cross(c));
}

inline void SWAP(double a, double b) 
{ 
	double tmp = b; 
	b = a; 
	a = tmp; 
}

int SolveQuadraticEqn(double a, double b, double c, double roots[2])
{
	if (a == 0) 
	{
		if (b == 0)
		{
			return 0;
		}
		else
		{
			roots[0] = -c / b;
			return 1;
		};
	}

	double disc = b * b - 4.0 * a * c;

	if (disc > 0)
	{
		if (b == 0)
		{
			double r = fabs (0.5 * sqrt (disc) / a);
			roots[0] = -r;
			roots[1] =  r;
		}
		else
		{
			double sgnb = (b > 0 ? 1.0 : -1.0);
			double temp = -0.5 * (b + sgnb * sqrt (disc));
			double r1 = temp / a ;
			double r2 = c / temp ;

			if (r1 < r2) 
			{
				roots[0] = r1 ;
				roots[1] = r2 ;
			} 
			else 
			{
				roots[0] = r2 ;
				roots[1] = r1 ;
			}
		}

		return 2;
	}
	else if (disc == 0) 
	{
		roots[0] = -0.5 * b / a;
		roots[1]= roots[0];
		return 1 ;
	}

	return 0;
}

int SolveCubicEqn(double a, double b, double c, double roots[3])
{
	double q = (a * a - 3.0 * b);
	double r = (2.0 * a * a * a - 9.0 * a * b + 27.0 * c);

	double Q = q / 9.0;
	double R = r / 54.0;

	double Q3 = Q * Q * Q;
	double R2 = R * R;

	double CR2 = 729.0 * r * r;
	double CQ3 = 2916.0 * q * q * q;

	if (R == 0 && Q == 0)
	{
		roots[0] = - a / 3.0;
		roots[1] = - a / 3.0;
		roots[2] = - a / 3.0;
		return 3 ;
	}
	else if (CR2 == CQ3) 
	{
		double sqrtQ = sqrt(Q);

		if (R > 0)
		{
			roots[0] = -2 * sqrtQ  - a / 3.0;
			roots[1] = sqrtQ - a / 3.0;
			roots[2] = sqrtQ - a / 3.0;
		}
		else
		{
			roots[0] = - sqrtQ  - a / 3.0;
			roots[1] = - sqrtQ - a / 3.0;
			roots[2] = 2 * sqrtQ - a / 3.0;
		}
		return 3 ;
	}
	else if (CR2 < CQ3) /* equivalent to R2 < Q3 */
	{
		double sqrtQ = sqrt(Q);
		double sqrtQ3 = sqrtQ * sqrtQ * sqrtQ;
		double theta = acos(R / sqrtQ3);
		double norm = -2.0 * sqrtQ;

		roots[0] = norm * cos(theta / 3.0) - a / 3.0;
		roots[1] = norm * cos((theta + 2.0 * PI_D) / 3.0) - a / 3.0;
		roots[2] = norm * cos((theta - 2.0 * PI_D) / 3.0) - a / 3.0;
      
		/* Sort roots in increasing order */
		if ( roots[0] > roots[1] )
			SWAP(roots[0], roots[1]) ;
      
		if ( roots[1] > roots[2] )
		{
			SWAP(roots[1], roots[2]) ;
          
			if ( roots[0] > roots[1] )
				SWAP(roots[0], roots[1]) ;
		}
      
		return 3;
	}
	else
	{
		double sgnR = (R >= 0 ? 1.0 : -1.0);
		double A = -sgnR * pow(std::abs(R) + sqrt(R2 - Q3), 1.0/3.0);
		double B = Q / A ;
		roots[0] = A + B - a / 3.0;
		return 1;
	}

	return 0;
}

//=========================================================================
// From ROS.org
inline double cbrt(double v)
{
  return pow(v, 1.0 / 3.0);
}

const double IEPSILON = 1e-5;
const double NEAR_ZERO_THRESHOLD = 1e-7;
const double CCD_RESOLUTION = 1e-7;

template<typename FPT> 
  bool roughlyEqual(FPT left, FPT right, FPT tolerance)
{
  return ((left < right + tolerance) && (left > right - tolerance));
}

bool isZero(double v)
{
  return roughlyEqual(v, (double)0, NEAR_ZERO_THRESHOLD);
}

int PolySolverSolveLinear(const double c[2], double s[1])
{
  if(isZero(c[1]))
    return 0;
  s[0] = - c[0] / c[1];
  return 1;
}

int PolySolverSolveQuadric(const double c[3], double s[2])
{
  double p, q, D;

  // make sure we have a d2 equation

  if(isZero(c[2]))
    return PolySolverSolveLinear(c, s);

  // normal for: x^2 + px + q
  p = c[1] / (2.0 * c[2]);
  q = c[0] / c[2];
  D = p * p - q;

  if(isZero(D))
  {
    // one double root
    s[0] = s[1] = -p;
	s[1] = s[0];
    return 1;
  }

  if(D < 0.0)
    // no real root
    return 0;
  else
  {
    // two real roots
    double sqrt_D = sqrt(D);
    s[0] = -sqrt_D - p;
    s[1] = sqrt_D - p;

    return 2;
  }
}

int PolySolverSolveCubic(const double c[4], double s[3])
{

  int i, num;
  double sub, A, B, C, sq_A, p, q, cb_p, D;
  const double ONE_OVER_THREE = 1 / 3.0;

  // make sure we have a d2 equation
  if(isZero(c[3]))
    return PolySolverSolveQuadric(c, s);

  // normalize the equation:x ^ 3 + Ax ^ 2 + Bx  + C = 0
  A = c[2] / c[3];
  B = c[1] / c[3];
  C = c[0] / c[3];

  // substitute x = y - A / 3 to eliminate the quadratic term: x^3 + px + q = 0
  sq_A = A * A;
  p = (-ONE_OVER_THREE * sq_A + B) * ONE_OVER_THREE;
  q = 0.5 * (2.0 / 27.0 * A * sq_A - ONE_OVER_THREE * A * B + C);

  // use Cardano's formula
  cb_p = p * p * p;
  D = q * q + cb_p;

  if(isZero(D))
  {
    if(isZero(q))
    {
      // one triple solution
      s[0] = 0.0;
	  s[1] = s[0];
	  s[2] = s[0];
      num = 1;
    }
    else
    {
      // one single and one double solution
      double u = cbrt(-q);
      s[0] = 2.0 * u;
      s[1] = -u;
	  s[2] = s[1];
      num = 2;
    }
  }
  else
  {
    if(D < 0.0)
    {
      // three real solutions
      double phi = ONE_OVER_THREE * acos(-q / sqrt(-cb_p));
      double t = 2.0 * sqrt(-p);
      s[0] = t * cos(phi);
      s[1] = -t * cos(phi + PI_D / 3.0);
      s[2] = -t * cos(phi - PI_D / 3.0);
      num = 3;
    }
    else
    {
      // one real solution
      double sqrt_D = sqrt(D);
      double u = cbrt(sqrt_D + fabs(q));
      if(q > 0.0)
        s[0] = - u + p / u ;
      else
        s[0] = u - p / u;

	  s[1] = s[0];
	  s[2] = s[0];

      num = 1;
    }
  }

  // re-substitute
  sub = ONE_OVER_THREE * A;
  for(i = 0; i < num; i++)
          s[i] -= sub;

  /* Sort roots in increasing order */
	if ( s[0] > s[1] )
		SWAP(s[0], s[1]) ;
      
	if ( s[1] > s[2] )
	{
		SWAP(s[1], s[2]) ;
          
		if ( s[0] > s[1] )
			SWAP(s[0], s[1]) ;
	}


  return num;
}

bool ROSSolveCubicWithIntervalNewton(double& l, double& r, double coeffs[])
{
  double v2[2]= {l*l,r*r};
  double v[2]= {l,r};
  double r_backup;

  unsigned char min3, min2, min1, max3, max2, max1;

  min3= *((unsigned char*)&coeffs[3]+7)>>7; max3=min3^1;
  min2= *((unsigned char*)&coeffs[2]+7)>>7; max2=min2^1;
  min1= *((unsigned char*)&coeffs[1]+7)>>7; max1=min1^1;

  // bound the cubic

  double minor = coeffs[3]*v2[min3]*v[min3]+coeffs[2]*v2[min2]+coeffs[1]*v[min1]+coeffs[0];
  double major = coeffs[3]*v2[max3]*v[max3]+coeffs[2]*v2[max2]+coeffs[1]*v[max1]+coeffs[0];

  if(major<0) return false;
  if(minor>0) return false;

  // starting here, the bounds have opposite values
  double m = 0.5 * (r + l);

  // bound the derivative
  double dminor = 3.0*coeffs[3]*v2[min3]+2.0*coeffs[2]*v[min2]+coeffs[1];
  double dmajor = 3.0*coeffs[3]*v2[max3]+2.0*coeffs[2]*v[max2]+coeffs[1];

  if((dminor > 0)||(dmajor < 0)) // we can use Newton
  {
    double m2 = m*m;
    double fm = coeffs[3]*m2*m+coeffs[2]*m2+coeffs[1]*m+coeffs[0];
    double nl = m;
    double nu = m;
    if(fm>0)
    {
      nl-=(fm/dminor);
      nu-=(fm/dmajor);
    }
    else
    {
      nu-=(fm/dminor);
      nl-=(fm/dmajor);
    }

    //intersect with [l,r]

    if(nl>r) return false;
    if(nu<l) return false;
    if(nl>l)
    {
      if(nu<r) { l=nl; r=nu; m=0.5*(l+r); }
      else { l=nl; m=0.5*(l+r); }
    }
    else
    {
      if(nu<r) { r=nu; m=0.5*(l+r); }
    }
  }

  // sufficient temporal resolution, check root validity
  if((r-l)< CCD_RESOLUTION)
  {
    return true;
  }

  r_backup = r, r = m;
  if(ROSSolveCubicWithIntervalNewton(l, r, coeffs))
    return true;

  l = m, r = r_backup;
  return ROSSolveCubicWithIntervalNewton(l, r, coeffs);
}

//==========================================================================================//




bool CCD_Filter(const CVector3D& a0, const CVector3D& b0, const CVector3D& c0, const CVector3D& d0,
				const CVector3D& a1, const CVector3D& b1, const CVector3D& c1, const CVector3D& d1)
{
	return true;

	CVector3D& n0 = norm(a0, b0, c0);
	CVector3D& n1 = norm(a1, b1, c1);
	CVector3D& delta = norm(a1-a0, b1-b0, c1-c0); //((b1-b0)-(a1-a0)).cross((c1-c0)-(a1-a0));
	CVector3D& nX = (n0+n1-delta)*0.5;

	CVector3D& pa0 = d0-a0;
	CVector3D& pa1 = d1-a1;

	double A = n0.Dot(pa0);
	double B = n1.Dot(pa1);
	double C = nX.Dot(pa0);
	double D = nX.Dot(pa1);
	double E = n1.Dot(pa0);
	double F = n0.Dot(pa1);

	if ( A > 0 && B > 0 && (2*C+F) > 0 && (2*D+E) > 0 )
		return false;

	if ( A < 0 && B < 0 && (2*C+F) < 0 && (2*D+E) < 0 )
		return false;

	return true;
}

inline double Clamp(double a, double l, double h)
{
   if ( a < l ) 
	   return l;
   else if( a > h ) 
	   return h;
   else 
	   return a;
}

// TODO: need to add , bool bClampToNearest
double DistancePointToEdge(const CVector3D& p, const CVector3D& x0, const CVector3D& x1, double& t, CVector3D& n)
{
	double d = 0;

	t = Clamp((x1-p).Dot(x1-x0)/(x1-x0).LengthSqr(), 0.0, 1.0); 
	CVector3D a = p - (t*x0 + (1.0-t)*x1);
	d = a.Length();
	n = a / (d + 1e-30);
	
	return d;
}

double DistanceEdgeToEdge(const CVector3D& x00, const CVector3D& x01, const CVector3D& x10, const CVector3D& x11, double& p, double& q, CVector3D& n, bool bClampToNearest/* = false*/)
{
	double d = 0;
	CVector3D e0 = x00 - x01;
	double l0 = e0.Length() + 1e-30;
	e0 = e0 / l0;

	CVector3D e1 = x11 - x10;
	double l1 = e1.Dot(e0);
	e1 = e1 - l1*e0;
	double l2 = e1.Length() + 1e-30;
	e1 = e1 / l2;
	CVector3D e2 = x11 - x01;
	q = e1.Dot(e2)/l2;
	p = (e0.Dot(e2)-l1*q)/l0;

	double tol = 1e-8;

	if ( p < -tol )
	{
		if ( bClampToNearest )
		{
			if ( q < -tol )
			{
				d = DistancePointToEdge(x01, x10, x11, q, n);
				n = (-1.0) * n;

				double dd = DistancePointToEdge(x11, x00, x01, p, n);

				if ( dd < d )
					d = dd;
			}
			else if( q > 1+tol )
			{			
				d = DistancePointToEdge(x01, x10, x11, q, n);
				n = (-1.0) * n;

				double dd = DistancePointToEdge(x10, x00, x01, p, n);

				if ( dd < d )
					d = dd;
			}
			else
			{
				p=0;
			
				d = DistancePointToEdge(x01, x10, x11, q, n);
				n = (-1.0) * n;
			}
		}
		else // the point is not on the edge and we ignore this case when bClampToNearest == false;
		{
			d = std::numeric_limits<double>::max();
			n = CVector3D(1.0, 0, 0); // no meaning at all.
		}
	}
	else if ( p > 1+tol )
	{
		if ( bClampToNearest )
		{
			if ( q < -tol ) 
			{
				d = DistancePointToEdge(x00, x10, x11, q, n);
				n = (-1.0) * n;

				double dd = DistancePointToEdge(x11, x00, x01, p, n);

				if ( dd < d )
					d = dd;
			}
			else if ( q > 1+tol )
			{
				d = DistancePointToEdge(x00, x10, x11, q, n);
				n = (-1.0) * n;

				double dd = DistancePointToEdge(x10, x00, x01, p, n);

				if ( dd < d )
					d = dd;
			}
			else
			{
				p = 1;
			
				d = DistancePointToEdge(x00, x10, x11, q, n);
				n = (-1.0) * n;
			}
		}
		else // the point is not on the edge and we ignore this case when bClampToNearest == false;
		{
			d = std::numeric_limits<double>::max();
			n = CVector3D(1.0, 0, 0); // no meaning at all.
		}
	}
	else
	{
		if ( q < -tol )
		{
			if ( bClampToNearest )
			{
				q = 0;			
				d = DistancePointToEdge(x11, x00, x01, p, n);
			}
			else // the point is not on the edge and we ignore this case when bClampToNearest == false;
			{
				d = std::numeric_limits<double>::max();
				n = CVector3D(1.0, 0, 0); // no meaning at all. 
		}
		}
		else if ( q > 1+tol )
		{
			if ( bClampToNearest )
			{
				q = 1;			
				d = DistancePointToEdge(x10, x00, x01, p, n);
			}
			else // the point is not on the edge and we ignore this case when bClampToNearest == false;
			{
				d = std::numeric_limits<double>::max();
				n = CVector3D(1.0, 0, 0); // no meaning at all. 
			}
		}
		else
		{ 
			n = (q*x10 + (1-q)*x11) - (p*x00 + (1-p)*x01);
			d = n.Length();
			
			if ( d > 0 ) 
				n = n / d;
			else // if d == 0
			{
				n = (x01-x00).Cross(x11-x10);
				n = n / (n.Length() + 1e-300);
			}
		}
	}

	p = Clamp(p, 0.0, 1.0);
	q = Clamp(q, 0.0, 1.0);
	
	return d;
}

double DistancePointToTriangle(const CVector3D& p, const CVector3D& x0, const CVector3D& x1, const CVector3D& x2,double& a, double& b, double& c, CVector3D& n, bool bClampToNearest/* = false*/)
{
	double d = 0;	
	CVector3D x02 = x0 - x2;
	double l0 = x02.Length() + 1e-30;
	x02 = x02 / l0;
	CVector3D x12 = x1 - x2;
    double l1 = x12.Dot(x02);
	x12 = x12 - l1*x02;
	double l2 = x12.Length() + 1e-30;
    x12 = x12 / l2;
    CVector3D px2 = p - x2;

	b = x12.Dot(px2) / l2;
	a = (x02.Dot(px2)-l1*b) / l0;
	c = 1 - a - b;

	double tol = 1e-8;

	if ( a >= -tol && b >= -tol && c >= -tol )
	{
		n = p - (a*x0 + b*x1 + c*x2);
		d = n.Length();

		if ( d > 0 ) 
			n = n / d;
		else // if d == 0
		{
			n  = (x1 - x0).Cross(x2 - x0);
			n = n / (n.Length() + 1e-300);
		}

		a = Clamp(a, 0.0, 1.0);
		b = Clamp(b, 0.0, 1.0);
		c = Clamp(c, 0.0, 1.0);
	}
	else
	{
		if ( !bClampToNearest )
		{
			d = std::numeric_limits<double>::max();
			n = CVector3D(1.0, 0, 0); // no meaning at all. 

			a = -1.0; 
			b = -1.0;
			c = -1.0;
		}
		else
		{
			double t;

			if ( a > 0 )
			{ 
				d = DistancePointToEdge(p, x0, x1, t, n);

				a = t; 
				b = 1.0 - t; 
				c = 0; 

				double dd = DistancePointToEdge(p, x0, x2, t, n);

				if ( dd < d )
				{
					a = t; 
					b = 0; 
					c = 1.0 - t;
					d = dd;
				}
			}
			else if ( b > 0 )
			{ 
				 d = DistancePointToEdge(p, x0, x1, t, n);

				 a = t; 
				 b = 1.0 - t; 
				 c = 0; 
         
				 double dd = DistancePointToEdge(p, x1, x2, t, n);

				 if ( dd < d )
				 {
					 a = 0; 
	 				 b = t; 
					 c = 1.0 - t;
					 d = dd;
				 }
			}
			else
			{ 
				d = DistancePointToEdge(p, x1, x2, t, n);

				a = 0; 
				b = t; 
				c = 1.0 - t; 

				double dd = DistancePointToEdge(p, x0, x2, t, n);

				if ( dd < d )			
				{
					a = t; 
					b = 0; 
					c = 1.0 - t;
					d = dd;
				}
			}
		}
	}

	return d;
}

bool ContinuousCollisionEdgeAndEdge(const CVector3D& x00_old, const CVector3D& x01_old, const CVector3D& x10_old, const CVector3D& x11_old,
	                      const CVector3D& x00_new, const CVector3D& x01_new, const CVector3D& x10_new, const CVector3D& x11_new, 
						  double& p, double& q, CVector3D& n, double& time)
{
	if ( !CCD_Filter(x00_old, x01_old, x10_old, x11_old, x00_new, x01_new, x10_new, x11_new) )
		return false;
		
	bool bCollide = false;

	// cubic equation, A*t^3+B*t^2+C*t+D = 0 such that t is in [0,1]
	CVector3D x10 = x01_old - x00_old;
	CVector3D x20 = x10_old - x00_old;
	CVector3D x30 = x11_old - x00_old;

	CVector3D v00 = x00_new - x00_old;
	CVector3D v10 = (x01_new - x01_old) - v00;
	CVector3D v20 = (x10_new - x10_old) - v00;
	CVector3D v30 = (x11_new - x11_old) - v00;

	double A = Triple(v10, v20, v30);
	double B = Triple(x10, v20, v30) + Triple(v10, x20, v30) + Triple(v10, v20, x30);
	double C = Triple(x10, x20, v30) + Triple(x10, v20, x30) + Triple(v10, x20, x30);
	double D = Triple(x10, x20, x30);

	double root[3];
	int num = 0;

	//if ( A == 0 )
	//{
	//	num = SolveQuadraticEqn(B, C, D, root);
	//}
	//else
	//{
	//	num = SolveCubicEqn(B/A, C/A, D/A, root);
	//}

	double coeffs[4];
	coeffs[3] = A, coeffs[2] = B, coeffs[1] = C, coeffs[0] = D;
	/*num = PolySolverSolveCubic(coeffs, root);*/

	double l = 0;
	double r = 1.0;
	if ( ROSSolveCubicWithIntervalNewton(l, r, coeffs) )
	{
		num = 1;
		root[0] = (l+r)*0.5;
	}

	for ( int i = 0; i < num; i++ )
	{
		double t = root[i];

		if ( t < -1e-8 || t > 1.0 + 1e-8 )
			continue;

		CVector3D x00t = (1.0-t)*x00_old + t*x00_new;
		CVector3D x01t = (1.0-t)*x01_old + t*x01_new;
		CVector3D x10t = (1.0-t)*x10_old + t*x10_new;
		CVector3D x11t = (1.0-t)*x11_old + t*x11_new;

		double d = DistanceEdgeToEdge(x00t, x01t, x10t, x11t, p, q, n, false);

		//if ( 0 <= p && p <= 1.0 && 0 <= q && q <= 1.0 )
		if ( d <= 1e-8 )
		{
			bCollide = true;

			if ( abs(time - t) > 0.001 )
				int sdfsdfsdfsdf = 0;

			time = t;
			break;
		}
	}

	return bCollide;
}

/* 
	p_old  : position of point in time step n
	x0_old : position of vertex 0 in trangle in time step n
	x1_old : position of vertex 1 in trangle in time step n
	x2_old : position of vertex 2 in triangle in time step n

	p_new  : position of point in time step n+1
	x0_new : position of vertex 0 in triangle in time step n+1
	x1_new : position of vertex 1 in triangle in time step n+1
	x2_new : position of vertex 2 in triangle in time step n+1

	a, b, c : barycentric coordinates of collision point in triangle between time step n and n+1. Output.
	n : normal vector of collision point. Output
	time : collision time in [0, 1], Output

	bClampToNearest : flag whether clamping the collision point to the triangle if it is out of triangle. Default is false. 
*/
bool ContinuousCollisionPointToTriangle(const CVector3D& p_old, const CVector3D& x0_old, const CVector3D& x1_old, const CVector3D& x2_old,
				                const CVector3D& p_new, const CVector3D& x0_new, const CVector3D& x1_new, const CVector3D& x2_new,
								double& a, double& b, double& c, CVector3D& n, double& time)
{
	if ( !CCD_Filter(x0_old, x1_old, x2_old, p_old, x0_new, x1_new, x2_new, p_new) )
		return false;

	bool bCollide = false;

	if ( 1 )
	{
		// cubic equation, A*t^3+B*t^2+C*t+D = 0 such that t is in [0,1]
		CVector3D x10 = x0_old - p_old;
		CVector3D x20 = x1_old - p_old;
		CVector3D x30 = x2_old - p_old;
		CVector3D v10 = (x0_new - x0_old) - (p_new - p_old);
		CVector3D v20 = (x1_new - x1_old) - (p_new - p_old);
		CVector3D v30 = (x2_new - x2_old) - (p_new - p_old);

		double A = Triple(v10, v20, v30);
		double B = Triple(x10, v20, v30) + Triple(v10, x20, v30) + Triple(v10, v20, x30);
		double C = Triple(x10, x20, v30) + Triple(x10, v20, x30) + Triple(v10, x20, x30);
		double D = Triple(x10, x20, x30);

		double root[3];
		int num = 0;

		//if ( A == 0 )
		//{
		//	num = SolveQuadraticEqn(B, C, D, root);
		//}
		//else
		//{
		//	num = SolveCubicEqn(B/A, C/A, D/A, root);
		//	//num = Solve3CubicBullet(A, B, C, D, root);
		//}

		double coeffs[4];
		coeffs[3] = A, coeffs[2] = B, coeffs[1] = C, coeffs[0] = D;
		//num = PolySolverSolveCubic(coeffs, root);

		//num = cubicRoots(B/A, C/A, D/A, root);

	
		double l = 0;
		double r = 1.0;
		if ( ROSSolveCubicWithIntervalNewton(l, r, coeffs) )
		{
			num = 1;
			root[0] = (l+r)*0.5;
		}

		for ( int i = 0; i < num; i++ )
		{
			double t = root[i];

			if ( t < -1e-8 || t > 1.0 + 1e-8 )
				continue;

			CVector3D p = (1.0-t)*p_old + t*p_new;
			CVector3D x0 = (1.0-t)*x0_old + t*x0_new;
			CVector3D x1 = (1.0-t)*x1_old + t*x1_new;
			CVector3D x2 = (1.0-t)*x2_old + t*x2_new;

			double d = DistancePointToTriangle(p, x0, x1, x2, a, b, c, n, false);

			if ( a >= 0 && b >= 0 && c >= 0 ) // if the intersecting point is inside of triangle
			{
				bCollide = true;
				time = t;
				break;
			}
		}
	}	

	return bCollide;
}
