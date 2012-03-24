#include <common/Physics/SimpleConvexShape.h>
#include <string.h>

#include <common/Utils/ConvexHullUtils.h>


float4* SimpleConvexShape::getVertexBuffer()
{
	return m_vtx;
}

int4* SimpleConvexShape::getTriangleBuffer()
{
	return m_faces;
}

int SimpleConvexShape::getNumVertex() const
{
	return m_nVtx;
}

int SimpleConvexShape::getNumTris() const
{
	return m_nFaces;
}

bool SimpleConvexShape::calcPenetration( const float4& v, float4& out, float maxDistance )
{
	ADLASSERT(0);
	return false;
}

bool SimpleConvexShape::set(const float4& pos, const Quaternion& quat, const float4* vtx, int nVtx, const int4* faces, int nFaces)
{
	if( nVtx >= MAX_VTX && nFaces >= MAX_FACES )
		return false;

	m_pos = pos;
	m_quat = quat;

	m_nVtx = nVtx;
	m_nFaces = nFaces;

	memcpy( m_vtx, vtx, m_nVtx*sizeof(float4) );
	memcpy( m_faces, faces, m_nFaces*sizeof(int4) );
	return true;
}


SimpleConvexShape* SimpleConvexShape::createRandomShape( float cvxRadius, float minRadius, int nPoints )
{
	SimpleConvexShape* cvx = new SimpleConvexShape();
	{
		Array<float4> points;
		Array<int4> tris;

		bool created = false;
		while( !created )
		{
			points.setSize( nPoints );
			float minH = -cvxRadius;
			float maxH = cvxRadius;
			for(int i=0; i<nPoints; i++)
			{
				points[i] = make_float4( getRandom(minH,maxH), getRandom(minH,maxH), getRandom(minH,maxH) );
				if( length3( points[i] ) < minRadius )
				{
					float4 normalized = normalize3(points[i]);
					points[i] = minRadius*normalized;
				}
			}
			tris.clear();
			ConvexHullUtils::createCvxShapeBF( points.begin(), points.getSize(), tris );

			created = cvx->set( make_float4(0), qtGetIdentity(), 
				points.begin(), points.getSize(), tris.begin(), tris.getSize() );
		}
	}
	return cvx;
}

bool SimpleConvexShape::operator == (const ShapeBase& input) const
{
	if( input.m_type == ShapeBase::SHAPE_SIMPLE_CVX ) return false;
	SimpleConvexShape* cvx = (SimpleConvexShape*)&input;
	if( cvx->m_nFaces != m_nFaces || cvx->m_nVtx != m_nVtx ) return false;

	for(int i=0; i<m_nVtx; i++)
	{
		if( cvx->m_vtx[i].x != m_vtx[i].x || cvx->m_vtx[i].y != m_vtx[i].y || cvx->m_vtx[i].z != m_vtx[i].z ) return false;
	}

	for(int i=0; i<m_nFaces; i++)
	{
		if( cvx->m_faces[i].x != m_faces[i].x || cvx->m_faces[i].y != m_faces[i].y || cvx->m_faces[i].z != m_faces[i].z ) return false;
	}

	return true;
}
