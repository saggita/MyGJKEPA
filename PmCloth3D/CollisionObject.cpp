#include "CollisionObject.h"
#include <GL/glut.h>

CCollisionObject::CCollisionObject(void) : m_HalfExtent(1.0), m_Margin(0.01)
{
	m_pBulletColObj = new btCollisionObject();	
}

CCollisionObject::~CCollisionObject(void)
{
	delete m_pBulletColObj;
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
	
	return supportPoint;
}

void CCollisionObject::Render() const
{
	GLfloat white[4] = {0.5, 0.5, 0.5, 1.0};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, white);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);

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

		glEnable(GL_DEPTH_TEST);
		glLineWidth(1.0f);
		glutSolidCube(1.0);

		glPushAttrib(GL_LIGHTING_BIT);
		
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glLineWidth(1.0f);
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(1, 0x0101);	// dash/dot/dash 
		glColor3f(1,1,1);
		glutWireCube(1);

		glDisable(GL_LINE_STIPPLE);
		glEnable(GL_DEPTH_TEST);
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
}


