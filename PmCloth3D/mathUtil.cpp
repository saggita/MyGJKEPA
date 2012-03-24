#include "mathUtil.h"
#include "Vector3D.h"

inline btScalar clamp(btScalar val, btScalar low, btScalar high)
{
	if ( val < low )
		return low;
	else if ( val > high )
		return high;

	return val;
}

// http://www.gamedev.net/topic/552906-closest-point-on-triangle/
btScalar DistanceFromPointToTriangle(const CVector3D& p, const CVector3D& p0, const CVector3D& p1, const CVector3D& p2, CVector3D* closestPointInTriangle/* = NULL*/)
{
	CVector3D edge0 = p1 - p0;
    CVector3D edge1 = p2 - p0;
    CVector3D v0 = p0 - p;

    btScalar a = edge0.Dot( edge0 );
    btScalar b = edge0.Dot( edge1 );
    btScalar c = edge1.Dot( edge1 );
    btScalar d = edge0.Dot( v0 );
    btScalar e = edge1.Dot( v0 );

    btScalar det = a*c - b*b;
    btScalar s = b*e - c*d;
    btScalar t = b*d - a*e;

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
            btScalar invDet = 1.f / det;
            s *= invDet;
            t *= invDet;
        }
    }
    else
    {
        if ( s < 0.f )
        {
            btScalar tmp0 = b+d;
            btScalar tmp1 = c+e;
            if ( tmp1 > tmp0 )
            {
                btScalar numer = tmp1 - tmp0;
                btScalar denom = a-2*b+c;
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
                btScalar numer = c+e-b-d;
                btScalar denom = a-2*b+c;
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
            btScalar numer = c+e-b-d;
            btScalar denom = a-2*b+c;
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
btScalar SignedDistanceFromPointToPlane(const CVector3D& point, const CVector3D& p0, const CVector3D& p1, const CVector3D& p2, CVector3D* closestPointInTriangle/* = NULL*/)
{
	CVector3D n = (p1-p0).Cross(p2-p0).Normalize();

	if ( n.LengthSqr() < 1e-6 )
		return 0;
	else
	{
		btScalar dist = (point-p0).Dot(n);

		if ( closestPointInTriangle )
			*closestPointInTriangle = point - dist * n;

		return dist;
	}
}

// Assumes planeEqn[0], planeEqn[1] and planeEqn[2] forms unit normal vector.
btScalar SignedDistanceFromPointToPlane(const CVector3D& point, const btScalar* planeEqn, CVector3D* closestPointInTriangle/* = NULL*/)
{
	CVector3D n(planeEqn[0], planeEqn[1], planeEqn[2]);

	if ( n.LengthSqr() < 1e-6 )
		return 0;
	else
	{
		btScalar dist = n.Dot(point) + planeEqn[3];

		if ( closestPointInTriangle )
			*closestPointInTriangle = point - dist * n;

		return dist;
	}
}
