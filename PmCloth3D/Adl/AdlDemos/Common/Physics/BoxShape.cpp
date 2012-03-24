#include <common/Physics/BoxShape.h>


BoxShape::BoxShape( const float4& halfExtent )
: ShapeBase( SHAPE_BOX )
{
	m_halfExtent = halfExtent;

	m_vtx[0] = make_float4( -halfExtent.x, -halfExtent.y, halfExtent.z );
	m_vtx[1] = make_float4( halfExtent.x, -halfExtent.y, halfExtent.z );
	m_vtx[2] = make_float4( halfExtent.x, -halfExtent.y, -halfExtent.z );
	m_vtx[3] = make_float4( -halfExtent.x, -halfExtent.y, -halfExtent.z );

	m_vtx[4] = make_float4( -halfExtent.x, halfExtent.y, halfExtent.z );
	m_vtx[5] = make_float4( halfExtent.x, halfExtent.y, halfExtent.z );
	m_vtx[6] = make_float4( halfExtent.x, halfExtent.y, -halfExtent.z );
	m_vtx[7] = make_float4( -halfExtent.x, halfExtent.y, -halfExtent.z );

	m_tris[0] = make_int4(0,3,2);
	m_tris[1] = make_int4(0,2,1);

	m_tris[2] = make_int4(4,0,1);
	m_tris[3] = make_int4(4,1,5);

	m_tris[4] = make_int4(5,1,2);
	m_tris[5] = make_int4(5,2,6);

	m_tris[6] = make_int4(6,2,3);
	m_tris[7] = make_int4(6,3,7);

	m_tris[8] = make_int4(7,3,0);
	m_tris[9] = make_int4(7,0,4);

	m_tris[10] = make_int4(7,4,5);
	m_tris[11] = make_int4(7,5,6);


	m_normals[0] = createEquation( m_vtx[0], m_vtx[3], m_vtx[1] );
	m_normals[1] = createEquation( m_vtx[4], m_vtx[0], m_vtx[1] );
	m_normals[2] = createEquation( m_vtx[5], m_vtx[1], m_vtx[2] );
	m_normals[3] = createEquation( m_vtx[6], m_vtx[2], m_vtx[3] );
	m_normals[4] = createEquation( m_vtx[7], m_vtx[3], m_vtx[4] );
	m_normals[5] = createEquation( m_vtx[7], m_vtx[4], m_vtx[5] );
}

BoxShape::~BoxShape()
{
}

bool BoxShape::calcPenetration( const float4& v, float4& out, float maxDistance )
{
	float height[6];
	for(int i=0; i<6; i++)
	{
		height[i] = dot3w1( v, m_normals[i] );
		if( height[i] > maxDistance ) return false;
	}

	int maxHeightIdx = -1;
	for(int i=0; i<6; i++)
	{
		if( height[i] > height[maxHeightIdx] )
			maxHeightIdx = i;
	}

	out = m_normals[maxHeightIdx];
	out.w = height[maxHeightIdx];
	return true;
}

Matrix3x3 BoxShape::calcInertia(const float4& extent, float mass)
{
	Matrix3x3 inertia = mtZero();
	(inertia.m_row[0].x) = 1.f/12.f*(pow(extent.y,2.f)+pow(extent.z,2.f))*mass;
	(inertia.m_row[1].y) = 1.f/12.f*(pow(extent.z,2.f)+pow(extent.x,2.f))*mass;
	(inertia.m_row[2].z) = 1.f/12.f*(pow(extent.x,2.f)+pow(extent.y,2.f))*mass;

	return inertia;
}

void BoxShape::get6FaceQuads(int4* idxOut)
{
	idxOut[0] = make_int4(0,3,2,1);
	idxOut[1] = make_int4(0,1,5,4);
	idxOut[2] = make_int4(1,2,6,5);
	idxOut[3] = make_int4(2,3,7,6);
	idxOut[4] = make_int4(3,0,4,7);
	idxOut[5] = make_int4(4,5,6,7);
}


bool BoxShape::operator == (const ShapeBase& input) const 
{
	if( input.m_type != ShapeBase::SHAPE_BOX ) return false;
	const BoxShape* box = (const BoxShape*)&input;
	return m_halfExtent.x == box->m_halfExtent.x && m_halfExtent.y == box->m_halfExtent.y && m_halfExtent.z == box->m_halfExtent.z;
}

