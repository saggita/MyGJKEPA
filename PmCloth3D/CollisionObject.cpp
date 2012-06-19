#ifdef WIN32
#include <windows.h>
#endif

#include <GL/glut.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <memory.h>
#include "CollisionObject.h"
#include "StringTokenizer.h"

static void DrawTextGlut(const char* str, float x, float y, float z, float r = 0.0f, float g = 0.0f, float b = 0.0f);

#define BarrelVtxCount2 57
#define BarrelIndexCount 60

static float BarrelVtx2[] = {
0.0f,-0.5f,0.0f,				0.0f,-1.0f,0.0f,
0.282362f,-0.5f,-0.205148f,     0.0f,-1.0f,0.0f,
0.349018f,-0.5f,0.0f,           0.0f,-1.0f,0.0f,
0.107853f,-0.5f,-0.331936f,     0.0f,-1.0f,0.0f,
-0.107853f,-0.5f,-0.331936f,    0.0f,-1.0f,0.0f,
0.107853f,-0.5f,-0.331936f,     0.0f,-1.0f,0.0f,
-0.282362f,-0.5f,-0.205148f,    0.0f,-1.0f,0.0f,
-0.349018f,-0.5f,0.0f,          0.0f,-1.0f,0.0f,
-0.282362f,-0.5f,0.205148f,     0.0f,-1.0f,0.0f,
-0.107853f,-0.5f,0.331936f,     0.0f,-1.0f,0.0f,
0.107853f,-0.5f,0.331936f,      0.0f,-1.0f,0.0f,
0.282362f,-0.5f,0.205148f,      0.0f,-1.0f,0.0f,
0.0f,0.5f,0.0f,                 0.0f,1.0f,0.0f,
0.349018f,0.5f,0.0f,            0.0f,1.0f,0.0f,
0.282362f,0.5f,-0.205148f,      0.0f,1.0f,0.0f,
0.107853f,0.5f,-0.331936f,      0.0f,1.0f,0.0f,
0.107853f,0.5f,-0.331936f,      0.0f,1.0f,0.0f,
-0.107853f,0.5f,-0.331936f,     0.0f,1.0f,0.0f,
-0.282362f,0.5f,-0.205148f,     0.0f,1.0f,0.0f,
-0.349018f,0.5f,0.0f,           0.0f,1.0f,0.0f,
-0.282362f,0.5f,0.205148f,      0.0f,1.0f,0.0f,
-0.107853f,0.5f,0.331936f,      0.0f,1.0f,0.0f,
0.107853f,0.5f,0.331936f,       0.0f,1.0f,0.0f,
0.282362f,0.5f,0.205148f,       0.0f,1.0f,0.0f,
0.349018f,-0.5f,0.0f,           0.957307f,-0.289072f,0.0f,
0.404509f,0.0f,-0.293893f,      0.809017f,0.0f,-0.587785f,
0.5f,0.0f,0.0f,                 1.0f,0.0f,0.0f,
0.282362f,-0.5f,-0.205148f,     0.774478f,-0.289072f,-0.562691f,
0.154508f,0.0f,-0.475528f,      0.309017f,0.0f,-0.951057f,
0.107853f,-0.5f,-0.331936f,     0.295824f,-0.289072f,-0.910453f,
0.107853f,-0.5f,-0.331936f,     0.295824f,-0.289072f,-0.910453f,
-0.154509f,0.0f,-0.475528f,     -0.309017f,0.0f,-0.951057f,
0.154508f,0.0f,-0.475528f,      0.309017f,0.0f,-0.951057f,
-0.107853f,-0.5f,-0.331936f,    -0.295824f,-0.289072f,-0.910453f,
-0.404509f,0.0f,-0.293893f,     -0.809017f,0.0f,-0.587785f,
-0.282362f,-0.5f,-0.205148f,    -0.774478f,-0.289072f,-0.562691f,
-0.5f,0.0f,0.0f,                -1.0f,0.0f,0.0f,
-0.349018f,-0.5f,0.0f,          -0.957307f,-0.289072f,0.0f,
-0.404508f,0.0f,0.293893f,      -0.809017f,0.0f,0.587785f,
-0.282362f,-0.5f,0.205148f,     -0.774478f,-0.289072f,0.562691f,
-0.154509f,0.0f,0.475528f,      -0.309017f,0.0f,0.951056f,
-0.107853f,-0.5f,0.331936f,     -0.295824f,-0.289072f,0.910453f,
0.154509f,0.0f,0.475528f,       0.309017f,0.0f,0.951056f,
0.107853f,-0.5f,0.331936f,      0.295824f,-0.289072f,0.910453f,
0.404509f,0.0f,0.293892f,       0.809017f,0.0f,0.587785f,
0.282362f,-0.5f,0.205148f,      0.774478f,-0.289072f,0.562691f,
0.282362f,0.5f,-0.205148f,      0.774478f,0.289072f,-0.562691f,
0.349018f,0.5f,0.0f,            0.957307f,0.289072f,0.0f,
0.107853f,0.5f,-0.331936f,      0.295824f,0.289072f,-0.910453f,
-0.107853f,0.5f,-0.331936f,     -0.295824f,0.289072f,-0.910453f,
0.107853f,0.5f,-0.331936f,      0.295824f,0.289072f,-0.910453f,
-0.282362f,0.5f,-0.205148f,     -0.774478f,0.289072f,-0.562691f,
-0.349018f,0.5f,0.0f,           -0.957307f,0.289072f,0.0f,
-0.282362f,0.5f,0.205148f,      -0.774478f,0.289072f,0.562691f,
-0.107853f,0.5f,0.331936f,      -0.295824f,0.289072f,0.910453f,
0.107853f,0.5f,0.331936f,       0.295824f,0.289072f,0.910453f,
0.282362f,0.5f,0.205148f,       0.774478f,0.289072f,0.562691f,
};


static int BarrelIdx[] = {
0,1,2,
0,3,1,
0,4,5,
0,6,4,
0,7,6,
0,8,7,
0,9,8,
0,10,9,
0,11,10,
0,2,11,
12,13,14,
12,14,15,
12,16,17,
12,17,18,
12,18,19,
12,19,20,
12,20,21,
12,21,22,
12,22,23,
12,23,13,
24,25,26,
24,27,25,
27,28,25,
27,29,28,
30,31,32,
30,33,31,
33,34,31,
33,35,34,
35,36,34,
35,37,36,
37,38,36,
37,39,38,
39,40,38,
39,41,40,
41,42,40,
41,43,42,
43,44,42,
43,45,44,
45,26,44,
45,24,26,
26,46,47,
26,25,46,
25,48,46,
25,28,48,
32,49,50,
32,31,49,
31,51,49,
31,34,51,
34,52,51,
34,36,52,
36,53,52,
36,38,53,
38,54,53,
38,40,54,
40,55,54,
40,42,55,
42,56,55,
42,44,56,
44,47,56,
44,26,47,
};

CTriangleFace::CTriangleFace() : m_bFlag(false)
{
	for ( int i = 0; i < 3; i++ )
	{
		m_IndexVrx[i] = -1;
		m_IndexEdge[i] = -1;			
	}
		
	m_Index = -1;
}

CTriangleFace::CTriangleFace(const CTriangleFace& other)
{
	for ( int i = 0; i < 3; i++ )
	{
		m_IndexVrx[i] = other.m_IndexVrx[i];
		m_IndexEdge[i] = other.m_IndexEdge[i];
	}

	for ( int i = 0; i < 4; i++ )
		m_PlaneEqn[i] = other.m_PlaneEqn[i];

	m_bFlag = other.m_bFlag;
	m_Index = other.m_Index;
}

CTriangleFace::~CTriangleFace() 
{
}

CTriangleFace& CTriangleFace::operator=(const CTriangleFace& other)
{
	for ( int i = 0; i < 3; i++ )
	{
		m_IndexVrx[i] = other.m_IndexVrx[i];
		m_IndexEdge[i] = other.m_IndexEdge[i];
	}

	for ( int i = 0; i < 4; i++ )
		m_PlaneEqn[i] = other.m_PlaneEqn[i];

	m_Index = other.m_Index;
	m_bFlag = other.m_bFlag;

	return (*this);
}



CCollisionObject::CCollisionObject() : m_HalfExtent(1.0), m_pConvexHeightField(NULL), m_bLoaded(false),
														  m_ddcl(NULL), m_ddhost(NULL)
{
	//m_pBulletColObj = new btCollisionObject();	

	SetColor(1.0, 1.0, 1.0);
}

CCollisionObject::CCollisionObject(Device* ddcl, Device* ddhost) : m_HalfExtent(1.0),  
																   m_pConvexHeightField(NULL), m_bLoaded(false),
																   m_ddcl(ddcl), m_ddhost(ddhost)
{
	//m_pBulletColObj = new btCollisionObject();	

	SetColor(1.0, 1.0, 1.0);
}

CCollisionObject::CCollisionObject(const CCollisionObject& other) : m_pConvexHeightField(NULL)
{
	m_CollisionObjectType = other.m_CollisionObjectType;
	m_Transform = other.m_Transform;
	m_HalfExtent = other.m_HalfExtent;
	
	for ( int i = 0; i < 4; i++ )
		m_Color[i] = other.m_Color[i];

	m_Vertices = other.m_Vertices;	
	m_Normals = other.m_Normals;
	m_Faces = other.m_Faces;
	m_Edges = other.m_Edges;

	m_VisualizedPoints = other.m_VisualizedPoints;
	
	// m_pConvexHeightField
	float4* eqn = new float4[m_Faces.size()];

	for ( int i=0; i < (int)m_Faces.size();i++ )
	{
		eqn[i].x = m_Faces[i].PlaneEquation()[0];
		eqn[i].y = m_Faces[i].PlaneEquation()[1];
		eqn[i].z = m_Faces[i].PlaneEquation()[2];
		eqn[i].w = m_Faces[i].PlaneEquation()[3];
	}
	
	m_pConvexHeightField = new ConvexHeightField(eqn, m_Faces.size());
	delete [] eqn;

	m_bLoaded = other.m_bLoaded;

	m_ddcl = other.m_ddcl; 
	m_ddhost = other.m_ddhost;	
	m_ShapeBuffer = other.m_ShapeBuffer;
	m_Data = other.m_Data;
	m_pBufRBodiesCPU = other.m_pBufRBodiesCPU;
	m_pBufRBodiesGPU = other.m_pBufRBodiesGPU;
}

CCollisionObject::~CCollisionObject(void)
{
	//delete m_pBulletColObj;

	if ( m_pConvexHeightField )
	{
		delete m_pConvexHeightField;
		m_pConvexHeightField = NULL;
	}
}

bool CCollisionObject::Create()
{

	return true;
}

const CTransform& CCollisionObject::GetTransform() const 
{ 
	return m_Transform; 
}

CTransform& CCollisionObject::GetTransform() 
{ 
	return m_Transform; 
}

//void CCollisionObject::SetTranslation(const CVector3D& translate)
//{
//	translation = translate;
//
//	btVector3 trans(translate.m_X, translate.m_Y, translate.m_Z);
//	m_pBulletColObj->getWorldTransform().setOrigin(trans);
//}
//
//void CCollisionObject::SetRotation(const CQuaternion& rotation)
//{
//	m_Rotation = rotation;
//
//	CMatrix33 mat;
//	m_Rotation.GetRotation(&mat);
//
//	btMatrix3x3 rot(mat(0, 0), mat(0, 1), mat(0, 2),
//					mat(1, 0), mat(1, 1), mat(1, 2),
//					mat(2, 0), mat(2, 1), mat(2, 2));
//
//	m_pBulletColObj->getWorldTransform().setBasis(rot);
//}

void CCollisionObject::SetCollisionObjectType(CollisionObjectType collisionObjectType) 
{ 
	m_CollisionObjectType = collisionObjectType; 

	if ( m_CollisionObjectType == ConvexHull )
	{
		if ( !m_bLoaded )
		{
			for ( int i = 0; i < BarrelVtxCount2; i++ )
			{
				CVertex vertex;
				CVector3D normal;
				vertex[0] = BarrelVtx2[i*6];
				vertex[1] = BarrelVtx2[i*6+1];
				vertex[2] = BarrelVtx2[i*6+2];

				normal[0] = BarrelVtx2[i*6+3];
				normal[1] = BarrelVtx2[i*6+4];
				normal[2] = BarrelVtx2[i*6+5];

				m_Normals.push_back(normal);
				m_Vertices.push_back(vertex);
			}

			for ( int i = 0; i < BarrelIndexCount; i++ )
			{
				int indices[3];
				indices[0] = BarrelIdx[i*3];
				indices[1] = BarrelIdx[i*3+1];
				indices[2] = BarrelIdx[i*3+2];
			
				CTriangleFace face;

				for ( int i = 0; i < 3; i++ )
					face.SetVertexIndex(i, indices[i]);

				m_Faces.push_back(face);
			}
		}
	}
}

CVector3D CCollisionObject::GetLocalSupportPoint(const CVector3D& dir, float margin/* = 0*/) const
{
	assert(margin >= 0.0);

	CVector3D supportPoint(0, 0, 0);

	if ( m_CollisionObjectType == Point )
	{
		if ( dir.LengthSqr() > 0.0 )
		{
			CVector3D dirN = dir;
			dirN.Normalize();
			supportPoint = margin*dirN;
		}
		else
			supportPoint.Set(margin, 0, 0);
	}
	if ( m_CollisionObjectType == LineSegment )
	{
		float maxDot = -FLT_MAX;
	
		for ( int i = 0; i < (int)m_Vertices.size(); i++ )
		{
			const CVector3D& vertex = m_Vertices[i];
			float dot = vertex.Dot(dir);

			if ( dot > maxDot )
			{
				supportPoint = vertex;
				maxDot = dot;
			}
		}
	}
	else if ( m_CollisionObjectType == Sphere )
	{
		float radius = m_HalfExtent.m_X;

		if ( dir.LengthSqr() > 0.0 )
			supportPoint = (radius + margin) * dir.NormalizeOther();
		else
			supportPoint = CVector3D(0, radius + margin, 0);
	}
	else if ( m_CollisionObjectType == Box )
	{		
		supportPoint.m_X = dir.m_X < 0 ? -m_HalfExtent.m_X - margin : m_HalfExtent.m_X + margin;
		supportPoint.m_Y = dir.m_Y < 0 ? -m_HalfExtent.m_Y - margin : m_HalfExtent.m_Y + margin;
		supportPoint.m_Z = dir.m_Z < 0 ? -m_HalfExtent.m_Z - margin : m_HalfExtent.m_Z + margin;
	}
	else if ( m_CollisionObjectType == Cone )
	{
		float radius = m_HalfExtent.m_X;
		float halfHeight = 2.0*m_HalfExtent.m_Y;
		float sinTheta = radius / (sqrt(radius * radius + 4 * halfHeight * halfHeight));
		const CVector3D& v = dir;
		float sinThetaTimesLengthV = sinTheta * v.Length();

		if ( v.m_Y >= sinThetaTimesLengthV) {
			supportPoint = CVector3D(0.0, halfHeight, 0.0);
		}
		else {
			float projectedLength = sqrt(v.m_X * v.m_X + v.m_Z * v.m_Z);
			if (projectedLength > 1e-10) {
				float d = radius / projectedLength;
				supportPoint = CVector3D(v.m_X * d, -halfHeight, v.m_Z * d);
			}
			else {
				supportPoint = CVector3D(radius, -halfHeight, 0.0);
			}
		}

		// Add the margin to the support point
		if (margin != 0.0) {
			CVector3D unitVec(0.0, -1.0, 0.0);
			if (v.LengthSqr() > 1e-10 * 1e-10) {
				unitVec = v.NormalizeOther();
			}
			supportPoint += unitVec * margin;
		}
	}
	else if ( m_CollisionObjectType == ConvexHull )
	{
		float maxDot = -FLT_MAX;
	
		for ( int i = 0; i < (int)m_Vertices.size(); i++ )
		{
			const CVector3D& vertex = m_Vertices[i];
			float dot = vertex.Dot(dir);

			if ( dot > maxDot )
			{
				supportPoint = vertex;
				maxDot = dot;
			}
		}
	}
	
	return supportPoint;
}

void CCollisionObject::Render(bool bWireframe/* = false*/) const
{
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, m_Color);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, m_Color);

	const CVector3D& translation = m_Transform.GetTranslation();
	const CQuaternion& rotation = m_Transform.GetRotation();

	CMatrix33 mat;
	rotation.GetRotation(&mat);

	if ( m_CollisionObjectType == Sphere )
	{
		GLdouble val[16];
		memset(val, 0, sizeof(GLdouble)*16);
		
		float x, y, z;
		x = m_HalfExtent.m_X;
		y = x;
		z = y;

		GLdouble rotMatrix[16];
		rotMatrix[0] = x*mat(0, 0); rotMatrix[4] = y*mat(0, 1); rotMatrix[8] =  z*mat(0, 2); rotMatrix[12] = translation.m_X;
		rotMatrix[1] = x*mat(1, 0); rotMatrix[5] = y*mat(1, 1); rotMatrix[9] =  z*mat(1, 2); rotMatrix[13] = translation.m_Y;
		rotMatrix[2] = x*mat(2, 0); rotMatrix[6] = y*mat(2, 1); rotMatrix[10] = z*mat(2, 2); rotMatrix[14] = translation.m_Z;
		rotMatrix[3] = 0;           rotMatrix[7] = 0;           rotMatrix[11] = 0;           rotMatrix[15] = 1;

		glMatrixMode(GL_MODELVIEW);

		glPushMatrix();
		glMultMatrixd(rotMatrix);

		glEnable(GL_DEPTH_TEST);
		glLineWidth(1.0f);
		glutSolidSphere(1.0, 16, 16);
		
		glPushAttrib(GL_LIGHTING_BIT);
		glDisable(GL_LIGHTING);

		glDisable(GL_DEPTH_TEST);
		glLineWidth(1.0f);
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(1, 0x0101);	// dash/dot/dash 
		glColor3f(1,1,1);
		glutWireSphere(1.0, 16, 16);

		glDisable(GL_LINE_STIPPLE);
		glEnable(GL_DEPTH_TEST);
		glLineWidth(1.0f);
		glutWireSphere(1.0, 16, 16);

		glLineWidth(1.0f);
		glPopAttrib();
		glPopMatrix();
	}
	else if ( m_CollisionObjectType == Point )
	{
		glPushMatrix();
		glTranslated(translation.m_X, translation.m_Y, translation.m_Z);
		glutSolidSphere(0.05, 6, 6);
		glPopMatrix();
	}
	else if ( m_CollisionObjectType == LineSegment )
	{
		GLdouble val[16];
		memset(val, 0, sizeof(GLdouble)*16);
		
		float x, y, z;
		x = 1.0;
		y = 1.0;
		z = 1.0;

		GLdouble rotMatrix[16];
		rotMatrix[0] = x*mat(0, 0); rotMatrix[4] = y*mat(0, 1); rotMatrix[8] =  z*mat(0, 2); rotMatrix[12] = translation.m_X;
		rotMatrix[1] = x*mat(1, 0); rotMatrix[5] = y*mat(1, 1); rotMatrix[9] =  z*mat(1, 2); rotMatrix[13] = translation.m_Y;
		rotMatrix[2] = x*mat(2, 0); rotMatrix[6] = y*mat(2, 1); rotMatrix[10] = z*mat(2, 2); rotMatrix[14] = translation.m_Z;
		rotMatrix[3] = 0;           rotMatrix[7] = 0;           rotMatrix[11] = 0;           rotMatrix[15] = 1;

		glMatrixMode(GL_MODELVIEW);

		glPushMatrix();
		glMultMatrixd(rotMatrix);

		glPushAttrib(GL_LIGHTING_BIT);
				
		glLineWidth(1.0f);
		
		// one edge for line segment
		assert(m_Vertices.size() == 2);
		
		glDisable(GL_LIGHTING);
		glColor3f(0,0,0);
		glBegin(GL_LINES);

		const CVector3D& vertex0 = m_Vertices[0];
		const CVector3D& vertex1 = m_Vertices[1];

		glVertex3d(vertex0.m_X, vertex0.m_Y, vertex0.m_Z);
		glVertex3d(vertex1.m_X, vertex1.m_Y, vertex1.m_Z);

		glEnd();
		
		glPopAttrib();
		
		glPopMatrix();
	}
	else if ( m_CollisionObjectType == Box )
	{
		GLdouble val[16];
		memset(val, 0, sizeof(GLdouble)*16);
		
		float x, y, z;
		x = 2.0* m_HalfExtent.m_X;
		y = 2.0* m_HalfExtent.m_Y;
		z = 2.0* m_HalfExtent.m_Z;

		GLdouble rotMatrix[16];
		rotMatrix[0] = x*mat(0, 0); rotMatrix[4] = y*mat(0, 1); rotMatrix[8] =  z*mat(0, 2); rotMatrix[12] = translation.m_X;
		rotMatrix[1] = x*mat(1, 0); rotMatrix[5] = y*mat(1, 1); rotMatrix[9] =  z*mat(1, 2); rotMatrix[13] = translation.m_Y;
		rotMatrix[2] = x*mat(2, 0); rotMatrix[6] = y*mat(2, 1); rotMatrix[10] = z*mat(2, 2); rotMatrix[14] = translation.m_Z;
		rotMatrix[3] = 0;           rotMatrix[7] = 0;           rotMatrix[11] = 0;           rotMatrix[15] = 1;

		glMatrixMode(GL_MODELVIEW);

		glPushMatrix();
		glMultMatrixd(rotMatrix);

		/*glEnable(GL_DEPTH_TEST);
		glLineWidth(1.0f);
		glutSolidCube(1.0);
		*/

		glPushAttrib(GL_LIGHTING_BIT);
		
		
		glDisable(GL_LIGHTING);
		/*glDisable(GL_DEPTH_TEST);
		glLineWidth(1.0f);
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(1, 0x0101);	// dash/dot/dash 
		glColor3f(0,0,1);
		glutWireCube(1);
		

		glDisable(GL_LINE_STIPPLE);
		glEnable(GL_DEPTH_TEST);*/
		glColor3f(0,0,1);
		glLineWidth(1.0f);
		glutWireCube(1);

		glPopAttrib();
		
		glPopMatrix();
	}
	else if ( m_CollisionObjectType == Cone )
	{
		GLdouble val[16];
		memset(val, 0, sizeof(GLdouble)*16);
		
		float x, y, z;
		x = 1.0;
		y = 1.0;
		z = 1.0;

		GLdouble rotMatrix[16];
		rotMatrix[0] = x*mat(0, 0); rotMatrix[4] = y*mat(0, 1); rotMatrix[8] =  z*mat(0, 2); rotMatrix[12] = translation.m_X;
		rotMatrix[1] = x*mat(1, 0); rotMatrix[5] = y*mat(1, 1); rotMatrix[9] =  z*mat(1, 2); rotMatrix[13] = translation.m_Y;
		rotMatrix[2] = x*mat(2, 0); rotMatrix[6] = y*mat(2, 1); rotMatrix[10] = z*mat(2, 2); rotMatrix[14] = translation.m_Z;
		rotMatrix[3] = 0;           rotMatrix[7] = 0;           rotMatrix[11] = 0;           rotMatrix[15] = 1;

		glMatrixMode(GL_MODELVIEW);

		glPushMatrix();
		glMultMatrixd(rotMatrix);
		glRotatef(-90.0f, 1.0, 0, 0);
		glutSolidCone(m_HalfExtent.m_X, 2.0*m_HalfExtent.m_Y, 16, 16);
		glPushAttrib(GL_LIGHTING_BIT);
		glDisable(GL_LIGHTING);
		glColor3f(1,1,1);
		glutWireCone(m_HalfExtent.m_X, 2.0*m_HalfExtent.m_Y, 16, 16);
		glPopAttrib();
		glPopMatrix();
	}
	else if ( m_CollisionObjectType == ConvexHull )
	{
		GLdouble val[16];
		memset(val, 0, sizeof(GLdouble)*16);
		
		float x, y, z;
		x = 1.0;
		y = 1.0;
		z = 1.0;

		GLdouble rotMatrix[16];
		rotMatrix[0] = x*mat(0, 0); rotMatrix[4] = y*mat(0, 1); rotMatrix[8] =  z*mat(0, 2); rotMatrix[12] = translation.m_X;
		rotMatrix[1] = x*mat(1, 0); rotMatrix[5] = y*mat(1, 1); rotMatrix[9] =  z*mat(1, 2); rotMatrix[13] = translation.m_Y;
		rotMatrix[2] = x*mat(2, 0); rotMatrix[6] = y*mat(2, 1); rotMatrix[10] = z*mat(2, 2); rotMatrix[14] = translation.m_Z;
		rotMatrix[3] = 0;           rotMatrix[7] = 0;           rotMatrix[11] = 0;           rotMatrix[15] = 1;

		glMatrixMode(GL_MODELVIEW);

		glPushMatrix();
		glMultMatrixd(rotMatrix);

		glPushAttrib(GL_LIGHTING_BIT);
				
		glLineWidth(1.0f);

		if ( !bWireframe )
		{
			// triangles
			for ( int i = 0; i < (int)m_Faces.size(); i++ )
			{
				const CTriangleFace& face = m_Faces[i];
			
				CVector3D normal(face.PlaneEquation()[0], face.PlaneEquation()[1], face.PlaneEquation()[2]);
				glNormal3d(normal.m_X, normal.m_Y, normal.m_Z);

				glBegin(GL_TRIANGLES);
			
				for ( int j = 0; j < 3; j++ )
				{
					const CVector3D& vertex = m_Vertices[face.GetVertexIndex(j)];
				
					glVertex3f(vertex.m_X, vertex.m_Y, vertex.m_Z);
				}

				glEnd();
			}
		}

		// edges
		glDisable(GL_LIGHTING);
		glColor3f(0,0,0);		

		for ( int i = 0; i < (int)m_Edges.size(); i++ )
		{
			const CEdge& edge = m_Edges[i];

			const CVector3D& vertex0 = m_Vertices[edge.GetVertexIndex(0)];
			const CVector3D& vertex1 = m_Vertices[edge.GetVertexIndex(1)];

			// sihlouette edges
			/*if ( edge.m_bFlag )
			{
				glLineWidth(3.0f);
				glColor3f(0, 0, 1.0f);	
			}
			else*/
			{
				glLineWidth(1.0f);
				glColor3f(0,0,0);	
			}

			glBegin(GL_LINE_STRIP);
				glVertex3f(vertex0.m_X, vertex0.m_Y, vertex0.m_Z);
				glVertex3f(vertex1.m_X, vertex1.m_Y, vertex1.m_Z);
			glEnd();

			char str[100];
			itoa(i, str,10);
			CVector3D middle = 0.5f * (vertex0 + vertex1);

			DrawTextGlut(str, middle.m_X, middle.m_Y, middle.m_Z);
		}

		// vertices
		/*for ( int i = 0; i < (int)m_Vertices.size(); i++ )
		{
			const CVertex& vert = m_Vertices[i];
			char str[100];
			sprintf(str, "%d(%3.2f, %3.2f, %3.2f)", i, vert.m_X, vert.m_Y, vert.m_Z);		

			DrawTextGlut(str, vert.m_X, vert.m_Y, vert.m_Z, 0, 0, 1.0f);
		}*/
				
		//// normal
		//glBegin(GL_LINES);

		//for ( int i = 0; i < (int)m_Faces.size(); i++ )
		//{
		//	const CTriangleFace& face = m_Faces[i];
		//	
		//	CVector3D normal(face.PlaneEquation()[0], face.PlaneEquation()[1], face.PlaneEquation()[2]);
		//	
		//	const CVector3D& v0 = m_Vertices[face.GetVertexIndex(0)];
		//	const CVector3D& v1 = m_Vertices[face.GetVertexIndex(1)];
		//	const CVector3D& v2 = m_Vertices[face.GetVertexIndex(2)];

		//	CVector3D c = (v0 + v1 + v2) * 0.3333333f;
		//	CVector3D n = c + normal;
		//	
		//	glVertex3f(c.m_X, c.m_Y, c.m_Z);
		//	glVertex3f(n.m_X, n.m_Y, n.m_Z);
		//}

		//glEnd();

		// convex heightfield visualization
		glPointSize(3.0f);
		glColor3f(1.0, 1.0, 0);
		glBegin(GL_POINTS);

		for ( int i = 0; i < (int)m_VisualizedPoints.size(); i++ )
		{
			const CVector3D& point = m_VisualizedPoints[i];
			glVertex3f(point.m_X, point.m_Y, point.m_Z);
		}

		glEnd();

		glPopAttrib();
		
		glPopMatrix();
	}
}

bool CCollisionObject::Load(const char* filename)
{
	m_bLoaded = false;

	m_Vertices.clear();
	m_Normals.clear();
	m_Faces.clear();
	m_Edges.clear();

	// Loading wavefront obj file.
	ifstream inFile(filename);
	string sLine;
	vector<string> sTokens;

	if ( !inFile.is_open() )
		return false;
		
	while (!inFile.eof() )
	{
		getline(inFile, sLine);
		sTokens.clear(); 
		int numFound = StringTokenizer(sLine, string(" "), sTokens, false);

		if ( numFound == 0 )
			continue;

		vector <string>::iterator iter;
		string sToken; 

		iter = sTokens.begin();
		sToken = *(iter);
		
		if ( sToken == "#" ) // comment
			continue;
		else if ( sToken == "v" ) // vertex
		{
			CVertex pnt;
			
			// x
			++iter;
			sToken = (*iter);			
			pnt.m_X = (float)atof(sToken.c_str());

			// y
			++iter;
			sToken = (*iter);			
			pnt.m_Y = (float)atof(sToken.c_str());

			// z
			++iter;
			sToken = (*iter);			
			pnt.m_Z = (float)atof(sToken.c_str());

			m_Vertices.push_back(pnt);
		}
		else if ( sToken == "vn" ) // vertex normal
		{
			CVector3D n;
			
			// x
			++iter;
			sToken = (*iter);			
			n.m_X = (float)atof(sToken.c_str());

			// y
			++iter;
			sToken = (*iter);			
			n.m_Y = (float)atof(sToken.c_str());

			// z
			++iter;
			sToken = (*iter);			
			n.m_Z = (float)atof(sToken.c_str());

			m_Normals.push_back(n);
		}
		else if ( sToken == "f" ) // face
		{
			CTriangleFace tri;
			vector<string> sTokens2;

			int i = 0;

			for ( iter = sTokens.begin() + 1; iter != sTokens.end(); iter++ )
			{
				sToken = (*iter);
				sTokens2.clear();
				numFound = StringTokenizer(sToken, string("/"), sTokens2, false);

				if ( numFound > 0 )
				{
					tri.SetVertexIndex(i++, atoi(sTokens2[0].c_str())-1);

					//if ( numFound == 3 )
					//	tri.m_IndexNormalVec = atoi(sTokens2[2].c_str());
				}
				else if ( numFound == 0 && sToken != "" )
				{
					tri.SetVertexIndex(i++, atoi(sToken.c_str())-1);
				}
			}		
			
			tri.SetIndex((int)m_Faces.size());
			m_Faces.push_back(tri);	
		}		
	}

	inFile.close();

	//-------
	// edges
	//-------
	for ( int i = 0; i < (int)m_Faces.size(); i++ )
	{
		CTriangleFace& tri = m_Faces[i];

		for ( int i = 0; i < 3; i++ )
		{
			int j = ((i != 2) ? i+1 : 0);

			CEdge edge(tri.GetVertexIndex(i), tri.GetVertexIndex(j));

			vector<CEdge>::iterator iterEdge = std::find(m_Edges.begin(), m_Edges.end(), edge);

			if ( iterEdge == m_Edges.end() )
			{
				edge.SetTriangleIndex(0, tri.GetIndex());

				edge.SetIndex((int)m_Edges.size());
				m_Edges.push_back(edge);

				m_Vertices[edge.GetVertexIndex(0)].SetEdgeIndex(edge.GetIndex());
				m_Vertices[edge.GetVertexIndex(1)].SetEdgeIndex(edge.GetIndex());
			}
			else
				(*iterEdge).SetTriangleIndex(1, tri.GetIndex());
		}		
	}

	for ( int i = 0; i < (int)m_Faces.size(); i++ )
	{
		CTriangleFace& tri = m_Faces[i];

		for ( int i = 0; i < 3; i++ )
		{
			int j = ((i != 2) ? i+1 : 0);

			CEdge edge(tri.GetVertexIndex(i), tri.GetVertexIndex(j));	
			vector<CEdge>::iterator iterEdge = std::find(m_Edges.begin(), m_Edges.end(), edge);

			if ( iterEdge == m_Edges.end() )
				assert(0); // must not reach here!

			tri.SetEdgeIndex(i, (*iterEdge).GetIndex());

			// set face index for vertices
			m_Vertices[tri.GetVertexIndex(i)].SetFaceIndex(tri.GetIndex());
		}		
	}

	// Compute plane equation for faces.
	for ( int i = 0; i < (int)m_Faces.size(); i++ )
	{
		CTriangleFace& face = m_Faces[i];

		const CVector3D& p0 = m_Vertices[face.GetVertexIndex(0)];
		const CVector3D& p1 = m_Vertices[face.GetVertexIndex(1)];
		const CVector3D& p2 = m_Vertices[face.GetVertexIndex(2)];

		CVector3D n = (p1-p0).Cross(p2-p0).Normalize();
		double d = -n.Dot(p0);

		face.PlaneEquation()[0] = n.m_X;
		face.PlaneEquation()[1] = n.m_Y;
		face.PlaneEquation()[2] = n.m_Z;
		face.PlaneEquation()[3] = d;
	}

	// Set up m_WindingOrderEdge
	for ( int i = 0; i < (int)m_Faces.size(); i++ )
	{
		CTriangleFace& tri = m_Faces[i];

		for ( int j = 0; j < 3; j++ )
		{
			CEdge& edge = m_Edges[tri.GetEdgeIndex(j)];

			int indexVertEdge0 = edge.GetVertexIndex(0);

			for ( int k = 0; k < 3; k++ )
			{
				if ( indexVertEdge0 == tri.GetVertexIndex(k) )
				{
					int l = (k < 2)? k+1 : 0; 

					if ( edge.GetVertexIndex(1) == tri.GetVertexIndex(l) )
						tri.SetWindingOrderEdge(j, true);
					else
						tri.SetWindingOrderEdge(j, false);
				}
			}
		}
	}

	if ( m_pConvexHeightField )
		delete m_pConvexHeightField;

	float4* eqn = new float4[m_Faces.size()];

	for ( int i=0; i < (int)m_Faces.size();i++ )
	{
		eqn[i].x = m_Faces[i].PlaneEquation()[0];
		eqn[i].y = m_Faces[i].PlaneEquation()[1];
		eqn[i].z = m_Faces[i].PlaneEquation()[2];
		eqn[i].w = m_Faces[i].PlaneEquation()[3];
	}
	
	m_pConvexHeightField = new ConvexHeightField(eqn, m_Faces.size());
	delete [] eqn;

	//VisualizeHF();

	if ( m_ddcl )
	{
		int numOfShapes = 1;
		m_ShapeBuffer = ChNarrowphase<TYPE_CL>::allocateShapeBuffer(m_ddcl, numOfShapes);	
		m_Data = ChNarrowphase<TYPE_CL>::allocate(m_ddcl);
	}

	m_bLoaded = true;
	return true;
}

void CCollisionObject::VisualizeHF()
{
	if ( !m_pConvexHeightField )
		return;
	
	m_VisualizedPoints.clear();

	srand(0);
	
	float del = 0.01;
	m_VisualizedPoints.reserve(0.5 * (1.0/(del*del)) * m_Faces.size());

	for ( int i = 0; i < (int)m_Faces.size(); i++ )
	{
		const CTriangleFace& face = m_Faces[i];

		const CVector3D& p0 = m_Vertices[face.GetVertexIndex(0)];
		const CVector3D& p1 = m_Vertices[face.GetVertexIndex(1)];
		const CVector3D& p2 = m_Vertices[face.GetVertexIndex(2)];
		
		for ( float a = 0; a <= 1.0; a += del )
		{
			for ( float b = 0; b <= 1.0; b += del )
			{
				float c = 1.0 - a - b;

				if ( 0 <= c && c <= 1.0 )
				{
					CVector3D p(a*p0 + b*p1 + c*p2);

					m_VisualizedPoints.push_back(p);
				}
			}
		}
	}

	for ( int i = 0; i < (int)m_VisualizedPoints.size(); i++ )
	{
		const CVector3D& point = m_VisualizedPoints[i];

		float4 normal;
		bool bNormal = m_pConvexHeightField->queryDistanceWithNormal(make_float4(point.m_X, point.m_Y, point.m_Z), normal);
		float dist = m_pConvexHeightField->queryDistance(make_float4(point.m_X, point.m_Y, point.m_Z));

		CVector3D closestPoint =  (point + CVector3D(normal.x, normal.y, normal.z) * (-dist));

		m_VisualizedPoints[i] = closestPoint;
	}

}

CCollisionObject& CCollisionObject::operator=(const CCollisionObject& other)
{
	m_CollisionObjectType = other.m_CollisionObjectType;
	m_Transform = other.m_Transform;
	m_HalfExtent = other.m_HalfExtent;
	
	for ( int i = 0; i < 4; i++ )
		m_Color[i] = other.m_Color[i];

	m_Vertices = other.m_Vertices;	
	m_Normals = other.m_Normals;
	m_Faces = other.m_Faces;
	m_Edges = other.m_Edges;

	m_VisualizedPoints = other.m_VisualizedPoints;
	
	// m_pConvexHeightField
	if ( m_pConvexHeightField )
		delete m_pConvexHeightField;

	float4* eqn = new float4[m_Faces.size()];

	for ( int i=0; i < (int)m_Faces.size();i++ )
	{
		eqn[i].x = m_Faces[i].PlaneEquation()[0];
		eqn[i].y = m_Faces[i].PlaneEquation()[1];
		eqn[i].z = m_Faces[i].PlaneEquation()[2];
		eqn[i].w = m_Faces[i].PlaneEquation()[3];
	}
	
	m_pConvexHeightField = new ConvexHeightField(eqn, m_Faces.size());
	delete [] eqn;

	m_bLoaded = other.m_bLoaded;

	m_ddcl = other.m_ddcl; 
	m_ddhost = other.m_ddhost;	
	m_ShapeBuffer = other.m_ShapeBuffer;
	m_Data = other.m_Data;
	m_pBufRBodiesCPU = other.m_pBufRBodiesCPU;
	m_pBufRBodiesGPU = other.m_pBufRBodiesGPU;

	return (*this);
}

void DrawTextGlut(const char* str, float x, float y) 
{
	glRasterPos2f(x, y);
	glColor3d(0.0, 0.0, 0.0);

	for (int i = 0; str[i] != '\0'; i++) 
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, str[i]);

}

void DrawTextGlut(const char* str, float x, float y, float z, float r, float g, float b) 
{
	glRasterPos3f(x, y, z);
	glColor3f(r, g, b);

	for (int i = 0; str[i] != '\0'; i++) 
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, str[i]);

}