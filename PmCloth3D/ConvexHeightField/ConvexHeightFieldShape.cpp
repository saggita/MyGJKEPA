#include "ConvexHeightFieldShape.h"
#include "CollideUtils.h"
#include <Demos/CubeMapUtils.h>
#include <common/Physics/ShapeBase.h>
#include <common/Physics/SphereShape.h>

#include <Demo.h>

//#define USE_OLD

namespace adl
{

ConvexHeightField::ConvexHeightField(const float4* vtxBuffer, const int4* idxBuffer, int nTriangles)
: CollisionShape( SHAPE_CONVEX_HEIGHT_FIELD )
{
	create( vtxBuffer, idxBuffer, nTriangles );
}

void ConvexHeightField::create( const float4* vtxBuffer, const int4* idxBuffer, int nTriangles )
{
	{
		float maxDx2 = -1.f;
		int maxIdx = -1;
		for(int i=0; i<nTriangles; i++)
		{
			const int4& idx = idxBuffer[i];
			for(int j=0; j<3; j++)
			{
				float dx2 = dot3F4( vtxBuffer[idx.s[j]], vtxBuffer[idx.s[j]] );
				if( dx2 > maxDx2 )
				{
					maxDx2 = dx2;
					maxIdx = idx.s[j];
				}
			}
		}
		ADLASSERT( maxIdx != -1 );
		m_scale = sqrtf( maxDx2 );
	}

	//	cast ray to find intersectPlaneLineions
	{
		for(u32 faceIdx=0; faceIdx<6; faceIdx++)
		{
			for(int i=0; i<HEIGHT_RES; i++) for(int j=0; j<HEIGHT_RES; j++)
			{
				float4 v;
				float x = (i+0.5f)/(float)HEIGHT_RES;
				float y = (j+0.5f)/(float)HEIGHT_RES;
				v = CubeMapUtils::calcVector(faceIdx, x, y);
				v = normalize3( v );
				v *= m_scale;

				float minFraction = FLT_MAX;
				float4 minNormal;
				float4 minBCrd;
				for(int itri=0; itri<nTriangles; itri++)
				{
					float4 from = make_float4(0.f);
					float4 bCrd;
					float fraction = CollideUtils::castRay( vtxBuffer[idxBuffer[itri].x], vtxBuffer[idxBuffer[itri].y], vtxBuffer[idxBuffer[itri].z], 
						from, v, 0.0f, &bCrd );

					if( fraction > 0.f )
					{
						minFraction = min2( minFraction, fraction );	//	todo. have to check if this is the min to replace normal?
						float4 ab = vtxBuffer[idxBuffer[itri].y]-vtxBuffer[idxBuffer[itri].x];
						float4 ac = vtxBuffer[idxBuffer[itri].z]-vtxBuffer[idxBuffer[itri].x];
						minNormal = cross3( ab, ac );
						minBCrd = bCrd;
					}
				}

				if( minFraction == FLT_MAX )
					minFraction = 0.f;

				{
					u8 quantizedHeight = (u8)(minFraction*255.f);
					sample( (Face)faceIdx, i,j ) = quantizedHeight;
					sampleNormal( (Face)faceIdx, i,j ) = normalize3(minNormal);
					float minValue = 3.f*(1.f/3.f)*(1.f/3.f);
					sampleNormal( (Face)faceIdx, i,j ).w = (dot3F4( minBCrd, minBCrd ) - minValue )/(1.f-minValue);
				}
			}
		}
	}

	calcSamplePoints( m_samplePoints );

	//	calc support height using m_samplePoints
	{
		for(u32 faceIdx=0; faceIdx<6; faceIdx++) for(int i=0; i<HEIGHT_RES; i++) for(int j=0; j<HEIGHT_RES; j++)
		{
			float4 v;
			float x = (i+0.5f)/(float)HEIGHT_RES;
			float y = (j+0.5f)/(float)HEIGHT_RES;
			v = CubeMapUtils::calcVector(faceIdx, x, y);
			v = normalize3( v );

			float maxHeight = -1;
			for(int ie=0; ie<6*HEIGHT_RES*HEIGHT_RES; ie++)
			{
				float h = dot3F4( v, m_samplePoints[ie] )/m_scale;
				ADLASSERT( h <= 1.f );
				if( h > maxHeight ) maxHeight = h;
			}

			{
				u8 quantizedHeight = min2((u8)(maxHeight*255.f)+1, 255);
				sampleSupport( (Face)faceIdx, i, j ) = quantizedHeight;
			}
		}
	}

	m_aabb.setEmpty();
	for(int i=0; i<nTriangles; i++)
	{
		const int4& idx = idxBuffer[i];
		m_aabb.includePoint( vtxBuffer[idx.x] );
		m_aabb.includePoint( vtxBuffer[idx.y] );
		m_aabb.includePoint( vtxBuffer[idx.z] );
	}
	m_aabb.expandBy( make_float4( m_collisionMargin ) );

	for(int i=0; i<6; i++)
	{
		m_faceAabbs[i].setEmpty();
		for(int j=0; j<HEIGHT_RES*HEIGHT_RES; j++)
		{
			float4 p = m_samplePoints[i*HEIGHT_RES*HEIGHT_RES + j];
			m_faceAabbs[i].includePoint(p);
		}
		m_faceAabbs[i].expandBy( make_float4( m_collisionMargin ) );
	}
}

ConvexHeightField::ConvexHeightField(const float4* eqn, int nEqn)
	: CollisionShape( SHAPE_CONVEX_HEIGHT_FIELD )
{
	{	//	cast ray to find intersectPlaneLineions
		for(u32 faceIdx=0; faceIdx<6; faceIdx++)
		{
			for(int i=0; i<HEIGHT_RES; i++) for(int j=0; j<HEIGHT_RES; j++)
			{
				float4 v;
				float x = (i+0.5f)/(float)HEIGHT_RES;
				float y = (j+0.5f)/(float)HEIGHT_RES;
				v = CubeMapUtils::calcVector(faceIdx, x, y);
				v = normalize3( v );

				float minFraction = FLT_MAX;
				float4 minNormal;
				for(int ii=0; ii<nEqn; ii++)
				{
					const float4& iEqn = eqn[ii];

					float fraction = intersectPlaneLine( iEqn, v, make_float4(0.f) );

					if( fraction > 0.f )
					{
						if( fraction < minFraction )
						{
							minFraction = fraction;
							minNormal = iEqn;
						}
					}
				}

				ADLASSERT( minFraction != FLT_MAX );

				minNormal.w = minFraction;
				sampleNormal( (Face)faceIdx, i, j ) = minNormal;
			}
		}
	}

	{
		m_scale = -FLT_MAX;
		for(u32 faceIdx=0; faceIdx<6; faceIdx++)
		{
			for(int i=0; i<HEIGHT_RES; i++) for(int j=0; j<HEIGHT_RES; j++)
			{
				float4& n = sampleNormal( (Face)faceIdx, i, j );

				m_scale = max2( m_scale, n.w );
			}
		}
		
		for(u32 faceIdx=0; faceIdx<6; faceIdx++)
		{
			for(int i=0; i<HEIGHT_RES; i++) for(int j=0; j<HEIGHT_RES; j++)
			{
				float4& n = sampleNormal( (Face)faceIdx, i, j );
				u8 quantizedHeight = (u8)(n.w/m_scale*255.f);
				sample( (Face)faceIdx, i, j ) = quantizedHeight;
			}
		}
	}

	calcSamplePoints( m_samplePoints );

	for(int i=0; i<6; i++)
	{
		m_faceAabbs[i].setEmpty();
		for(int j=0; j<HEIGHT_RES*HEIGHT_RES; j++)
		{
			float4 p = m_samplePoints[i*HEIGHT_RES*HEIGHT_RES + j];
			m_faceAabbs[i].includePoint(p);
		}
		m_faceAabbs[i].expandBy( make_float4( m_collisionMargin ) );
	}

	m_aabb.setEmpty();
	for(int i=0; i<6; i++)
	{
		m_aabb.includeVolume( m_faceAabbs[i] );
	}
}

ConvexHeightField::ConvexHeightField(const ShapeBase* shape)
	: CollisionShape( SHAPE_CONVEX_HEIGHT_FIELD )
{
	if( shape->m_type == ShapeBase::SHAPE_SPHERE )
	{
		SphereShape* sphere = (SphereShape*)shape;

		m_scale = sphere->m_radius;
		for(u32 faceIdx=0; faceIdx<6; faceIdx++)
		{
			for(int i=0; i<HEIGHT_RES; i++) for(int j=0; j<HEIGHT_RES; j++)
			{
				float4 minNormal;
				float x = (i+0.5f)/(float)HEIGHT_RES;
				float y = (j+0.5f)/(float)HEIGHT_RES;
				minNormal = CubeMapUtils::calcVector(faceIdx, x, y);
				minNormal = normalize3( minNormal );
				{
					u8 quantizedHeight = (u8)(1.f*255.f);
					sample( (Face)faceIdx, i,j ) = quantizedHeight;
					sampleNormal( (Face)faceIdx, i,j ) = normalize3(minNormal);
//					float minValue = 3.f*(1.f/3.f)*(1.f/3.f);
//					sampleNormal( (Face)faceIdx, i,j ).w = (dot3F4( minBCrd, minBCrd ) - minValue )/(1.f-minValue);
				}
			}
		}

		calcSamplePoints( m_samplePoints );

		m_aabb.m_max = make_float4( sphere->m_radius );
		m_aabb.m_min = make_float4( -sphere->m_radius );

		m_aabb.expandBy( make_float4( m_collisionMargin ) );

		for(int i=0; i<6; i++)
		{
			m_faceAabbs[i].setEmpty();
			for(int j=0; j<HEIGHT_RES*HEIGHT_RES; j++)
			{
				float4 p = m_samplePoints[i*HEIGHT_RES*HEIGHT_RES + j];
				m_faceAabbs[i].includePoint(p);
			}
			m_faceAabbs[i].expandBy( make_float4( m_collisionMargin ) );
		}
	}
	else
	{
		ShapeBase* s = (ShapeBase*)shape;

		create( s->getVertexBuffer(), s->getTriangleBuffer(), s->getNumTris() );
	}
}

ConvexHeightField::~ConvexHeightField()
{

}

float ConvexHeightField::queryDistance(const float4& p ) const
{
	const float4 majorAxes[] = {make_float4(1,0,0,0), make_float4(0,1,0,0), make_float4(0,0,1,0)};

	if( dot3F4( p, p ) >= m_scale*m_scale ) return FLT_MAX;

	int faceIdx;
	float x, y;
	CubeMapUtils::calcCrd( p, faceIdx, x, y );
	x = (x*HEIGHT_RES) - 0.5f;
	y = (y*HEIGHT_RES) - 0.5f;

	float height;
	{
		int xi = (int)(x);
		int yi = (int)(y);
		float dx = x-xi;
		float dy = y-yi;

		{
			int xip = min2((int)(HEIGHT_RES-1), xi+1);
			int yip = min2((int)(HEIGHT_RES-1), yi+1);

			u8 xy = sample( (Face)faceIdx, xi, yi );
			u8 xpy = sample( (Face)faceIdx, xip, yi );
			u8 xpyp = sample( (Face)faceIdx, xip, yip );
			u8 xyp = sample( (Face)faceIdx, xi, yip );

			height = (xy*(1.f-dx)+xpy*dx)*(1.f-dy) + (xyp*(1.f-dx)+xpyp*dx)*dy;
			height = height/255.f*m_scale;

			height = length3( p ) - height;
		}
	}

	return height;
}

float ConvexHeightField::querySupportHeight(const float4& p ) const
{
	const float4 majorAxes[] = {make_float4(1,0,0,0), make_float4(0,1,0,0), make_float4(0,0,1,0)};

//	if( dot3F4( p, p ) >= m_scale*m_scale ) return FLT_MAX;

	int faceIdx;
	float x, y;
	CubeMapUtils::calcCrd( p, faceIdx, x, y );
	x = (x*HEIGHT_RES) - 0.5f;
	y = (y*HEIGHT_RES) - 0.5f;

	float height;
	{
		int xi = (int)(x);
		int yi = (int)(y);
		float dx = x-xi;
		float dy = y-yi;

		{
			int xip = min2((int)(HEIGHT_RES-1), xi+1);
			int yip = min2((int)(HEIGHT_RES-1), yi+1);

			u8 xy = sampleSupport( (Face)faceIdx, xi, yi );
			u8 xpy = sampleSupport( (Face)faceIdx, xip, yi );
			u8 xpyp = sampleSupport( (Face)faceIdx, xip, yip );
			u8 xyp = sampleSupport( (Face)faceIdx, xi, yip );

			height = max2( xy, max2( xpy, max2( xpyp, xyp ) ) );
			height = height/255.f*m_scale;
		}
	}

	return height;
}

float ConvexHeightField::queryW(const float4& p ) const
{
	const float4 majorAxes[] = {make_float4(1,0,0,0), make_float4(0,1,0,0), make_float4(0,0,1,0)};

	float value;
	if( dot3F4( p, p ) >= m_scale*m_scale ) return 0;

	int faceIdx;
	float x, y;
	CubeMapUtils::calcCrd( p, faceIdx, x, y );
	x = (x*HEIGHT_RES) - 0.5f;
	y = (y*HEIGHT_RES) - 0.5f;

	{
		int xi = (int)(x);
		int yi = (int)(y);

		value = sampleNormal( (Face)faceIdx, xi, yi ).w;
	}
	return value;
}

bool ConvexHeightField::queryDistanceWithNormal( const float4& p, float4& normalOut ) const
{
	int faceIdx;
	float x, y;
	CubeMapUtils::calcCrd( p, faceIdx, x, y );
	x = (x*HEIGHT_RES) - 0.5f;
	y = (y*HEIGHT_RES) - 0.5f;

	{
		int xi = (int)(x);
		int yi = (int)(y);

		normalOut = sampleNormal( (Face)faceIdx, xi, yi );
	}
	return true;
}

void ConvexHeightField::calcSamplePoints(float4* points) const
{
	for(u32 faceIdx=0; faceIdx<6; faceIdx++)
	{
		for(int i=0; i<HEIGHT_RES; i++) for(int j=0; j<HEIGHT_RES; j++)
		{
			float4 v;
			float x = (i+0.5f)/(float)HEIGHT_RES;
			float y = (j+0.5f)/(float)HEIGHT_RES;
			v = CubeMapUtils::calcVector(faceIdx, x, y);
			v = normalize3( v );

			int quantizedHeight = sample( (Face)faceIdx, i, j );
			float rheight = quantizedHeight/255.f*m_scale;

			points[ HEIGHT_RES*HEIGHT_RES*faceIdx + i + j*HEIGHT_RES ] = rheight*v;
		}
	}
	return;
}

float4 ConvexHeightField::calcSamplePoint( int sIdx ) const
{
	int idir; int plus;
	Face faceIdx = (Face)(sIdx/(HEIGHT_RES*HEIGHT_RES));
	idir = (faceIdx/2);
	plus = faceIdx & 1;

	float4 viewVector = make_float4((idir==0)?1.f:0.f, (idir==1)?1.f:0.f, (idir==2)?1.f:0.f );
	if( plus==0 ) viewVector *= -1.f;
	float4 xVector = make_float4( viewVector.z, viewVector.x, viewVector.y );
	float4 yVector = make_float4( viewVector.y, viewVector.z, viewVector.x );
	float4 orig = viewVector-xVector-yVector;

	int pIdx = sIdx%(HEIGHT_RES*HEIGHT_RES);
	int i = pIdx/HEIGHT_RES;
	int j = pIdx%HEIGHT_RES;

	float4 v = orig + (i+0.5f)*xVector/(HEIGHT_RES*0.5f) + (j+0.5f)*yVector/(HEIGHT_RES*0.5f);
	v = normalize3( v );

	int quantizedHeight = sample( faceIdx, i, j );
	float rheight = quantizedHeight/255.f*m_scale;
	return rheight*v;
}

const float4* ConvexHeightField::getSamplePoints() const
{
	return m_samplePoints;
}

void ConvexHeightField::displaySamples(const float4& translation, const Quaternion& quaternion) const
{
	float4 c[HEIGHT_RES*HEIGHT_RES*6];
//	float4 v[HEIGHT_RES*HEIGHT_RES*6];

	float s = 0.3f;
	for(int i=0; i<HEIGHT_RES*HEIGHT_RES*6; i++)
	{
		const float4& n = m_normal[i];
		s = n.w;
		c[i] = rainbowMap( s );
		c[i] = make_float4(1.f);
//		v[i] = calcSamplePoint( i );
	}

	pxDrawPointListTransformed( (float4*)m_samplePoints, c, HEIGHT_RES*HEIGHT_RES*6, translation, quaternion );
//	pxDrawPointListTransformed( v, c, HEIGHT_RES*HEIGHT_RES*6, translation, quaternion );

//	pxDrawPointListTransformed( m_samplePoints, c, HEIGHT_RES*HEIGHT_RES, translation, quaternion );
}

};
