#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN
#  undef NOMINMAX
#endif

#include <GL/gl.h>
#include "Aabb.h"
#include <algorithm>
#include <limits>
#include "Point3D.h"

#include <assert.h>

using namespace std;

CAabb::CAabb(void)
{
	Empty();
}

CAabb::CAabb(const CAabb& other)
{
	m_Min = other.m_Min;
	m_Max = other.m_Max;
}

CAabb::~CAabb(void)
{
}

void CAabb::Set(CVector3D min, CVector3D max)
{
	m_Min = min;
	m_Max = max;
}

void CAabb::Enlarge(double h)
{
	m_Min.m_X -= h;
	m_Min.m_Y -= h;
	m_Min.m_Z -= h;

	m_Max.m_X += h;
	m_Max.m_Y += h;
	m_Max.m_Z += h;
}

bool CAabb::Collide(const IBoundingVolume& other, double tolerance /*= 0*/) const
{
	const CAabb& box = (CAabb&)other;

	if ( m_Min.m_X > box.m_Max.m_X + tolerance )
		return false;

	if ( m_Min.m_Y > box.m_Max.m_Y + tolerance )
		return false;

	if ( m_Min.m_Z > box.m_Max.m_Z + tolerance )
		return false;

	if ( m_Max.m_X < box.m_Min.m_X + tolerance )
		return false;

	if ( m_Max.m_Y < box.m_Min.m_Y + tolerance )
		return false;

	if ( m_Max.m_Z < box.m_Min.m_Z + tolerance )
		return false;

	return true;
}

bool CAabb::Inside(const CVector3D& point) const
{
	if ( point.m_X < m_Min.m_X || m_Max.m_X < point.m_X )
		return false;

	if ( point.m_Y < m_Min.m_Y || m_Max.m_Y < point.m_Y )
		return false;

	if ( point.m_Z < m_Min.m_Z || m_Max.m_Z < point.m_Z )
		return false;

	return true;
}

void CAabb::Empty()
{
	double max = numeric_limits<double>::max();
	double min = numeric_limits<double>::min();

	m_Min.Set(max, max, max);
	m_Max.Set(min, min, min);
}

bool CAabb::IsEmpty() const
{
	double max = numeric_limits<double>::max();
	double min = numeric_limits<double>::min();

	if ( m_Min.m_X == max && 
		 m_Min.m_Y == max && 
		 m_Min.m_Z == max && 
		 m_Max.m_X == min && 
		 m_Max.m_Y == min && 
		 m_Max.m_Z == min )
		 return true;
	else
		return false;
}

double CAabb::Height() const
{
	return m_Max.m_Y - m_Min.m_Y;
}

double CAabb::Width() const
{
	return m_Max.m_X - m_Min.m_X;
}

double CAabb::Length() const
{
	return m_Max.m_Z - m_Min.m_Z;
}

CVector3D CAabb::Center() const
{
	return (m_Min + m_Max) * 0.5;
}

double CAabb::Volume() const
{
	return Width() * Length() * Height();
}

int CAabb::LongestSide() const
{
	double w = Width();
	double h = Height();
	double l = Length();

	if ( w >= h && w >= l )
		return 0;
	else if ( h >= w && h >= l )
		return 1;
	else // if ( l >= w && l >= h )
		return 2;
}

void CAabb::Split(IBoundingVolume*& leftBV, IBoundingVolume*& rightBV) const
{
	leftBV = new CAabb(*this);
	rightBV = new CAabb(*this);

	CAabb* lBox = (CAabb*)leftBV;
	CAabb* rBox = (CAabb*)rightBV;
	
	CVector3D c = Center();

	int longSide = LongestSide();

	if ( longSide == 0 )
	{
		lBox->Max().m_X = c.m_X;
		rBox->Min().m_X = c.m_X;
	}
	else if ( longSide == 1 )
	{
		lBox->Max().m_Y = c.m_Y;
		rBox->Min().m_Y = c.m_Y;
	}
	else // if ( longSide == 2 )
	{
		lBox->Max().m_Z = c.m_Z;
		rBox->Min().m_Z = c.m_Z;
	}
}

void CAabb::Visualize(bool bCollided) const
{
	if ( !bCollided )
		return;

	CPoint3D min(m_Min);
	CPoint3D max(m_Max);

	if ( bCollided )
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(0.5f, 0.5f, 0.5f);

	glLineWidth(1.0);

	glBegin(GL_LINE_STRIP);
		glVertex3d(min.m_X, min.m_Y, min.m_Z);
		glVertex3d(max.m_X, min.m_Y, min.m_Z);
		glVertex3d(max.m_X, min.m_Y, max.m_Z);
		glVertex3d(min.m_X, min.m_Y, max.m_Z);
		glVertex3d(min.m_X, min.m_Y, min.m_Z);
	glEnd();

	glBegin(GL_LINE_STRIP);
		glVertex3d(min.m_X, max.m_Y, min.m_Z);
		glVertex3d(max.m_X, max.m_Y, min.m_Z);
		glVertex3d(max.m_X, max.m_Y, max.m_Z);
		glVertex3d(min.m_X, max.m_Y, max.m_Z);
		glVertex3d(min.m_X, max.m_Y, min.m_Z);
	glEnd();

	glBegin(GL_LINES);
		glVertex3d(min.m_X, min.m_Y, min.m_Z);
		glVertex3d(min.m_X, max.m_Y, min.m_Z);

		glVertex3d(max.m_X, min.m_Y, min.m_Z);
		glVertex3d(max.m_X, max.m_Y, min.m_Z);

		glVertex3d(max.m_X, min.m_Y, max.m_Z);
		glVertex3d(max.m_X, max.m_Y, max.m_Z);

		glVertex3d(min.m_X, min.m_Y, max.m_Z);
		glVertex3d(min.m_X, max.m_Y, max.m_Z);
	glEnd();
}

IBoundingVolume& CAabb::operator=(const IBoundingVolume& other)
{
	const CAabb& bv = (CAabb&)other;
	return operator=(bv);
}

CAabb& CAabb::operator=(const CAabb& other)
{
	m_Min = other.m_Min;
	m_Max = other.m_Max;

	return (*this);
}

IBoundingVolume& CAabb::operator+=(const CVector3D& vec)
{
	if ( IsEmpty() )
	{
		m_Min.m_X = vec.m_X;
		m_Min.m_Y = vec.m_Y;
		m_Min.m_Z = vec.m_Z;

		m_Max.m_X = vec.m_X;
		m_Max.m_Y = vec.m_Y;
		m_Max.m_Z = vec.m_Z;
	}
	else
	{
		for ( int i = 0; i < 3; i++ )
		{
			m_Min[i] = std::min(m_Min[i], vec[i]);
			m_Max[i] = std::max(m_Max[i], vec[i]);
		}
	}

	return (*this);
}

IBoundingVolume& CAabb::operator+=(const IBoundingVolume& other)
{
	const CAabb& bv = (CAabb&)other;

	if ( IsEmpty() )
	{
		*this = other;
	}
	else
	{
		for ( int i = 0; i < 3; i++ )
		{
			m_Min[i] = std::min(m_Min[i], bv.m_Min[i]);
			m_Max[i] = std::max(m_Max[i], bv.m_Max[i]);
		}
	}

	return (*this);
}