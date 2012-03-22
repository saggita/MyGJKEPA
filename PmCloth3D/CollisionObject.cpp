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

CCollisionObject::CCollisionObject(void) : m_HalfExtent(1.0), m_Margin(0.01)
{
	//m_pBulletColObj = new btCollisionObject();	

	SetColor(1.0, 1.0, 1.0);
}

CCollisionObject::~CCollisionObject(void)
{
	//delete m_pBulletColObj;
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
		Load("./model/geoSphere.obj");

		/*for ( int i = 0; i < BarrelVtxCount2; i++ )
		{
			CVector3D vertex, normal;
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
			
			TriangleFace face;

			for ( int i = 0; i < 3; i++ )
				face.indices[i] = indices[i];

			m_Faces.push_back(face);
		}*/
	}
}

CVector3D CCollisionObject::GetLocalSupportPoint(const CVector3D& dir, double margin/* = 0*/) const
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
	else if ( m_CollisionObjectType == Sphere )
	{
		double radius = m_HalfExtent.m_X;

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
		double radius = m_HalfExtent.m_X;
		double halfHeight = 2.0*m_HalfExtent.m_Y;
		double sinTheta = radius / (sqrt(radius * radius + 4 * halfHeight * halfHeight));
		const CVector3D& v = dir;
		double sinThetaTimesLengthV = sinTheta * v.Length();

		if ( v.m_Y >= sinThetaTimesLengthV) {
			supportPoint = CVector3D(0.0, halfHeight, 0.0);
		}
		else {
			double projectedLength = sqrt(v.m_X * v.m_X + v.m_Z * v.m_Z);
			if (projectedLength > 1e-10) {
				double d = radius / projectedLength;
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
		double maxDot = -DBL_MAX;
	
		for ( int i = 0; i < (int)m_Vertices.size(); i++ )
		{
			const CVector3D& vertex = m_Vertices[i];
			double dot = vertex.Dot(dir);

			if ( dot > maxDot )
			{
				supportPoint = vertex;
				maxDot = dot;
			}
		}
	}
	
	return supportPoint;
}

void CCollisionObject::Render() const
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
		
		double x, y, z;
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
	else if ( m_CollisionObjectType == Box )
	{
		GLdouble val[16];
		memset(val, 0, sizeof(GLdouble)*16);
		
		double x, y, z;
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
		
		double x, y, z;
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
		
		double x, y, z;
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
		
		glColor3f(0,0,1);
		glLineWidth(1.0f);
		
		/*glBegin(GL_TRIANGLES);

		for ( int i = 0; i < (int)m_Faces.size(); i++ )
		{
			const TriangleFace& face = m_Faces[i];

			for ( int j = 0; j < 3; j++ )
			{
				const CVector3D& vertex = m_Vertices[face.indices[j]];
				const CVector3D& normal = m_Normals[face.indices[j]];

				glNormal3d(normal.m_X, normal.m_Y, normal.m_Z);
				glVertex3d(vertex.m_X, vertex.m_Y, vertex.m_Z);
			}
		}

		glEnd();*/

		glDisable(GL_LIGHTING);
		glBegin(GL_LINES);

		for ( int i = 0; i < (int)m_Faces.size(); i++ )
		{
			const TriangleFace& face = m_Faces[i];

			for ( int j = 0; j < 3; j++ )
			{
				const CVector3D& vertex = m_Vertices[face.indices[j]];
				glVertex3d(vertex.m_X, vertex.m_Y, vertex.m_Z);
			}

			const CVector3D& vertex = m_Vertices[face.indices[0]];
			glVertex3d(vertex.m_X, vertex.m_Y, vertex.m_Z);

			// normal
			/*const CVector3D& v0 = m_Vertices[face.indices[0]];
			const CVector3D& v1 = m_Vertices[face.indices[1]];
			const CVector3D& v2 = m_Vertices[face.indices[2]];

			const CVector3D& normal = (v1-v0).Cross(v2-v0).Normalize();
			CVector3D n = vertex + normal;
			glVertex3d(n.m_X, n.m_Y, n.m_Z);*/
		}

		glEnd();

		glPopAttrib();
		
		glPopMatrix();
	}
}

bool CCollisionObject::Load(const char* filename)
{
	// Loading wavefront obj file.
	ifstream inFile(filename);
	string sLine;
	vector<string> sTokens;

	if ( !inFile.is_open() )
		return false;

	m_Vertices.clear();
	m_Normals.clear();
	m_Faces.clear();

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
			CVector3D pnt;
			
			// x
			++iter;
			sToken = (*iter);			
			pnt.m_X = (double)atof(sToken.c_str());

			// y
			++iter;
			sToken = (*iter);			
			pnt.m_Y = (double)atof(sToken.c_str());

			// z
			++iter;
			sToken = (*iter);			
			pnt.m_Z = (double)atof(sToken.c_str());

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
			TriangleFace tri;
			vector<string> sTokens2;

			int i = 0;

			for ( iter = sTokens.begin() + 1; iter != sTokens.end(); iter++ )
			{
				sToken = (*iter);
				sTokens2.clear();
				numFound = StringTokenizer(sToken, string("/"), sTokens2, false);

				if ( numFound > 0 )
				{
					tri.indices[i++] = atoi(sTokens2[0].c_str())-1;

					//if ( numFound == 3 )
					//	tri.m_IndexNormalVec = atoi(sTokens2[2].c_str());
				}
				else if ( numFound == 0 && sToken != "" )
				{
					tri.indices[i++] = atoi(sToken.c_str())-1;
				}
			}		

			m_Faces.push_back(tri);	
		}		
	}

	inFile.close();
}
