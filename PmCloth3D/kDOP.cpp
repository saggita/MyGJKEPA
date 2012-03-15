#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN
#  undef NOMINMAX
#endif

#include <GL/gl.h>
#include <algorithm>
#include <limits>
#include "kDop.h"

CkDOP18::CkDOP18(void)
{
	Empty();
}

CkDOP18::CkDOP18(const CkDOP18& other)
{
	for ( int i = 0; i < 18; i++ )
		d[i] = other.d[i];
}

CkDOP18::CkDOP18(const CVector3D& p)
{
	d[0] = d[9]  = p.m_X;
	d[1] = d[10] = p.m_Y;
	d[2] = d[11] = p.m_Z;

	Distances(p, d[3], d[4], d[5], d[6], d[7], d[8]);

	d[12] = d[3];
	d[13] = d[4];
	d[14] = d[5];
	d[15] = d[6];
	d[16] = d[7];
	d[17] = d[8];
}

CkDOP18::~CkDOP18(void)
{
}

void CkDOP18::Distances(const CVector3D& p, double& d3, double& d4, double& d5, double& d6, double& d7, double& d8) const
{
	d3 = p.m_X + p.m_Y;
	d4 = p.m_X + p.m_Z;
	d5 = p.m_Y + p.m_Z;
	d6 = p.m_X - p.m_Y;
	d7 = p.m_X - p.m_Z;
	d8 = p.m_Y - p.m_Z;
}

void CkDOP18::Distances(const CVector3D& p, double d[]) const
{
	d[0] = p.m_X + p.m_Y;
	d[1] = p.m_X + p.m_Z;
	d[2] = p.m_Y + p.m_Z;
	d[3] = p.m_X - p.m_Y;
	d[4] = p.m_X - p.m_Z;
	d[5] = p.m_Y - p.m_Z;
}

void CkDOP18::Enlarge(double h)
{
/*
	(-1,0,0) and (1,0,0)  -> indices 0 and 9
	(0,-1,0) and (0,1,0)  -> indices 1 and 10
	(0,0,-1) and (0,0,1)  -> indices 2 and 11
	(-1,-1,0) and (1,1,0) -> indices 3 and 12
	(-1,0,-1) and (1,0,1) -> indices 4 and 13
	(0,-1,-1) and (0,1,1) -> indices 5 and 14
	(-1,1,0) and (1,-1,0) -> indices 6 and 15
	(-1,0,1) and (1,0,-1) -> indices 7 and 16
	(0,-1,1) and (0,1,-1) -> indices 8 and 17
*/

	d[0] += -h; 
	d[1] += -h; 
	d[2] += -h;

	d[9] += h;
	d[10] += h;
	d[11] += h;

	d[3] += -h;
	d[4] += -h;
	d[5] += -h;
	d[6] += -h;
	d[7] += -h;
	d[8] += -h;

	d[12] += h;
	d[13] += h;
	d[14] += h;
	d[15] += h;
	d[16] += h;
	d[17] += h;
}

bool CkDOP18::Collide(const IBoundingVolume& other, double tolerance/* = 0*/) const
{
	const CkDOP18& dop = (CkDOP18&)other;

	for (int i = 0; i < 9; i++ ) 
	{
		if ( d[i] > dop.d[i+9] + tolerance) 
			return false;
			
		if ( d[i+9] < dop.d[i] + tolerance) 
			return false;
	}

	return true;
}

bool CkDOP18::Inside(const CVector3D& point) const
{
	for (int i = 0; i < 3; i++ ) 
	{
		if (point[i] < d[i] || point[i] > d[i+9])
			return false;
	}

	double a[6];
	Distances(point, a);
		
	for ( int i = 3; i < 9; i++ ) 
	{
		if (a[i-3] < d[i] || a[i-3] > d[i+9])
			return false;
	}

	return true;
}

void CkDOP18::Empty()
{
	for ( int i = 0; i < 9; i++ )
	{
		d[i] = std::numeric_limits<double>::max();
		d[i+9] = std::numeric_limits<double>::min();
	}
}

bool CkDOP18::IsEmpty() const
{
	double max = std::numeric_limits<double>::max();
	double min = std::numeric_limits<double>::min();

	for ( int i = 0; i < 9; i++ )
	{
		if ( d[i] != std::numeric_limits<double>::max() )
			return false;

		if ( d[i+9] != std::numeric_limits<double>::min() )
			return false;
	}

	return true;
}

void CkDOP18::Visualize(bool bCollided/* = false*/) const
{
	//if ( !bCollided )
	//	return;

	CVector3D min(d[0], d[1], d[2]);
	CVector3D max(d[9], d[10], d[11]);

	if ( bCollided )
		glColor3f(1.0f, 1.0f, 0.0f);
	else
		glColor3f(0.1f, 0.1f, 0.1f);

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

double CkDOP18::Height() const
{
	return d[10] - d[1];
}

double CkDOP18::Width() const
{
	return d[9] - d[0];
}

double CkDOP18::Length() const
{
	return d[11] - d[2];
}

CVector3D CkDOP18::Center() const
{
	return CVector3D(d[0]+d[9], d[1]+d[10], d[2]+d[11])*0.5;
}

double CkDOP18::Volume() const
{
	return Width() * Height() * Length();
}

// If width is longest, returns 0. If height is longest, returns 1. If length is longest, returns 2. 
int CkDOP18::LongestSide() const
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

// Split this into two CkDOPs by cutting the longest side half
void CkDOP18::Split(IBoundingVolume*& leftBV, IBoundingVolume*& rightBV) const
{
	leftBV = new CkDOP18(*this);
	rightBV = new CkDOP18(*this);

	CkDOP18* lBv = (CkDOP18*)leftBV;
	CkDOP18* rBv = (CkDOP18*)rightBV;
	
	CVector3D c = Center();
	int longSide = LongestSide();

	lBv->d[longSide + 9] = c[longSide];
	rBv->d[longSide] = c[longSide];
}

IBoundingVolume& CkDOP18::operator=(const IBoundingVolume& other)
{
	const CkDOP18& bv = (CkDOP18&)other;
	return operator=(bv);
}

CkDOP18& CkDOP18::operator=(const CkDOP18& other)
{
	for ( int i = 0; i < 18; i++ )
		d[i] = other.d[i];

	return (*this);
}

IBoundingVolume& CkDOP18::operator+=(const CVector3D& vec)
{
	if ( IsEmpty() )
	{
		for ( int i = 0; i < 3; i++ )
			d[i] = d[i+9] = vec[i];

		double a[6];
		Distances(vec, a);

		for ( int i = 0; i < 6; i++ )
			d[i+3] = d[i+12] = a[i];
	}
	else
	{
		d[0]  = std::min(vec[0], d[0]);
		d[9]  = std::max(vec[0], d[9]);
		d[1]  = std::min(vec[1], d[1]);
		d[10] = std::max(vec[1], d[10]);
		d[2]  = std::min(vec[2], d[2]);
		d[11] = std::max(vec[2], d[11]);

		double d3, d4, d5, d6, d7, d8;
		Distances(vec, d3, d4, d5, d6, d7, d8);

		d[3]  = std::min(d3, d[3]);
		d[12] = std::max(d3, d[12]);
		d[4]  = std::min(d4, d[4]);
		d[13] = std::max(d4, d[13]);
		d[5]  = std::min(d5, d[5]);
		d[14] = std::max(d5, d[14]);
		d[6]  = std::min(d6, d[6]);
		d[15] = std::max(d6, d[15]);
		d[7]  = std::min(d7, d[7]);
		d[16] = std::max(d7, d[16]);
		d[8]  = std::min(d8, d[8]);
		d[17] = std::max(d8, d[17]);
	}

	return (*this);
}

IBoundingVolume& CkDOP18::operator+=(const IBoundingVolume& other)
{
	const CkDOP18& bv = (CkDOP18&)other;

	if ( IsEmpty() )
	{
		(*this) = bv;
	}
	else
	{
		for ( int i = 0; i < 9; i++ )
		{
			d[i]  = std::min(bv.d[i], d[i]);
			d[i+9]  = std::max(bv.d[i+9], d[i+9]);
		}
	}
	
	return (*this);
}

