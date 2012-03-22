#include "mathUtil.h"
#include "Vector3D.h"

inline double clamp(double val, double low, double high)
{
	if ( val < low )
		return low;
	else if ( val > high )
		return high;

	return val;
}

// http://www.gamedev.net/topic/552906-closest-point-on-triangle/
double DistanceFromPointToTriangle(const CVector3D& p, const CVector3D& p0, const CVector3D& p1, const CVector3D& p2, CVector3D* closestPointInTriangle/* = NULL*/)
{
	CVector3D edge0 = p1 - p0;
    CVector3D edge1 = p2 - p0;
    CVector3D v0 = p0 - p;

    double a = edge0.Dot( edge0 );
    double b = edge0.Dot( edge1 );
    double c = edge1.Dot( edge1 );
    double d = edge0.Dot( v0 );
    double e = edge1.Dot( v0 );

    double det = a*c - b*b;
    double s = b*e - c*d;
    double t = b*d - a*e;

    if ( s + t < det )
    {
        if ( s < 0.f )
        {
            if ( t < 0.f )
            {
                if ( d < 0.f )
                {
                    s = clamp( -d/a, 0.f, 1.f );
                    t = 0.f;
                }
                else
                {
                    s = 0.f;
                    t = clamp( -e/c, 0.f, 1.f );
                }
            }
            else
            {
                s = 0.f;
                t = clamp( -e/c, 0.f, 1.f );
            }
        }
        else if ( t < 0.f )
        {
            s = clamp( -d/a, 0.f, 1.f );
            t = 0.f;
        }
        else
        {
            double invDet = 1.f / det;
            s *= invDet;
            t *= invDet;
        }
    }
    else
    {
        if ( s < 0.f )
        {
            double tmp0 = b+d;
            double tmp1 = c+e;
            if ( tmp1 > tmp0 )
            {
                double numer = tmp1 - tmp0;
                double denom = a-2*b+c;
                s = clamp( numer/denom, 0.f, 1.f );
                t = 1-s;
            }
            else
            {
                t = clamp( -e/c, 0.f, 1.f );
                s = 0.f;
            }
        }
        else if ( t < 0.f )
        {
            if ( a+d > b+e )
            {
                double numer = c+e-b-d;
                double denom = a-2*b+c;
                s = clamp( numer/denom, 0.f, 1.f );
                t = 1-s;
            }
            else
            {
                s = clamp( -e/c, 0.f, 1.f );
                t = 0.f;
            }
        }
        else
        {
            double numer = c+e-b-d;
            double denom = a-2*b+c;
            s = clamp( numer/denom, 0.f, 1.f );
            t = 1.f - s;
        }
    }

	CVector3D closestPoint = p0 + s * edge0 + t * edge1;

    if ( closestPointInTriangle )
		*closestPointInTriangle = closestPoint;

	return (p - closestPoint).Length();
}



