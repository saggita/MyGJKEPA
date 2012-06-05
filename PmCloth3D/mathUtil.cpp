#include "mathUtil.h"
#include "Vector3D.h"

inline float clamp(float val, float low, float high)
{
	if ( val < low )
		return low;
	else if ( val > high )
		return high;

	return val;
}

// http://www.gamedev.net/topic/552906-closest-point-on-triangle/
float DistanceFromPointToTriangle(const CVector3D& p, const CVector3D& p0, const CVector3D& p1, const CVector3D& p2, CVector3D* closestPointInTriangle/* = NULL*/)
{
	CVector3D edge0 = p1 - p0;
    CVector3D edge1 = p2 - p0;
    CVector3D v0 = p0 - p;

    float a = edge0.Dot( edge0 );
    float b = edge0.Dot( edge1 );
    float c = edge1.Dot( edge1 );
    float d = edge0.Dot( v0 );
    float e = edge1.Dot( v0 );

    float det = a*c - b*b;
    float s = b*e - c*d;
    float t = b*d - a*e;

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
            float invDet = 1.f / det;
            s *= invDet;
            t *= invDet;
        }
    }
    else
    {
        if ( s < 0.f )
        {
            float tmp0 = b+d;
            float tmp1 = c+e;
            if ( tmp1 > tmp0 )
            {
                float numer = tmp1 - tmp0;
                float denom = a-2*b+c;
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
                float numer = c+e-b-d;
                float denom = a-2*b+c;
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
            float numer = c+e-b-d;
            float denom = a-2*b+c;
            s = clamp( numer/denom, 0.f, 1.f );
            t = 1.f - s;
        }
    }

	CVector3D closestPoint = p0 + s * edge0 + t * edge1;

    if ( closestPointInTriangle )
		*closestPointInTriangle = closestPoint;

	return (p - closestPoint).Length();
}

// The normal vector of plane formed by p0, p1 and p2 is (p1-p0).Cross(p2-p0).Normalize().
// If 'point' is on the positive side of plane, it returns positive distance. Otherwise, it returns negative distance. 
float SignedDistanceFromPointToPlane(const CVector3D& point, const CVector3D& p0, const CVector3D& p1, const CVector3D& p2, CVector3D* closestPointInTriangle/* = NULL*/)
{
	CVector3D n = (p1-p0).Cross(p2-p0).Normalize();

	if ( n.LengthSqr() < 1e-6 )
		return 0;
	else
	{
		float dist = (point-p0).Dot(n);

		if ( closestPointInTriangle )
			*closestPointInTriangle = point - dist * n;

		return dist;
	}
}

// Assumes planeEqn[0], planeEqn[1] and planeEqn[2] forms unit normal vector.
float SignedDistanceFromPointToPlane(const CVector3D& point, const float* planeEqn, CVector3D* closestPointInTriangle/* = NULL*/)
{
	CVector3D n(planeEqn[0], planeEqn[1], planeEqn[2]);

	if ( n.LengthSqr() < 1e-6 )
		return 0;
	else
	{
		float dist = n.Dot(point) + planeEqn[3];

		if ( closestPointInTriangle )
			*closestPointInTriangle = point - dist * n;

		return dist;
	}
}
