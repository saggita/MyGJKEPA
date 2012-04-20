#include <omp.h>

#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN
#  undef NOMINMAX
#endif

#include <iostream>
#include <fstream>
#include <GL/gl.h>
#include <sstream>
#include <algorithm>

#include "Hair.h"
#include "StringTokenizer.h"
#include "NarrowPhaseCollisionDetection.h"
#include "ConvexCollisionAlgorithm.h"
#include "CollisionObject.h"

using namespace std;


CHair::CHair(void) : m_bDeformable(true), m_Gravity(0.0f, -9.8f, 0.0f), m_dt(0.0f)
{
	m_Kst = 10000.0f;
	m_Ksh = 10000.0f;
	m_Kb = 5000.0f;
	m_Kd = 0.0f;
	m_Mu = 0.3f;
	
	m_bEqualVertexMass = true;
	m_NumIterForConstraintSolver = 1;
}

CHair::CHair(const CHair& other)
{
	m_Kst = other.m_Kst;
	m_Ksh = other.m_Ksh;
	m_Kb = other.m_Kb;
	m_Kd = other.m_Kd;
	m_Gravity = other.m_Gravity;

	m_VertexArray = other.m_VertexArray;
	m_StrechSpringArray = other.m_StrechSpringArray;
	m_BendSpringArray = other.m_BendSpringArray;

	m_bDeformable = other.m_bDeformable;
	m_bEqualVertexMass = other.m_bEqualVertexMass;
}

CHair::~CHair(void)
{
	Clear();
}

void CHair::Clear()
{
	m_VertexArray.clear();
	m_StrechSpringArray.clear();
	m_BendSpringArray.clear();

}

void CHair::Initialize()
{
	m_bDeformable = true;

	GenerateBatches();
}

bool CHair::Load(const char* filename)
{
	// Loading wavefront obj file.
	ifstream inFile(filename);
	string sLine;
	vector<string> sTokens;

	if ( !inFile.is_open() )
		return false;

	m_VertexArray.clear();
	m_StrechSpringArray.clear();
	m_BendSpringArray.clear();

	getline(inFile, sLine);
	sTokens.clear(); 
	int numFound = StringTokenizer(sLine, string(" "), sTokens, false);

	if ( numFound == 0 )
		return false;

	// numStrands
	int numStrands = 0;

	vector <string>::iterator iter;
	string sToken; 

	iter = sTokens.begin();
	sToken = *(iter);

	assert(sToken == "numStrands");

	++iter;
	sToken = *(iter);

	numStrands = atoi(sToken.c_str());
	
	getline(inFile, sLine); // is sorted 1

	for ( int strand = 0; strand < numStrands; strand++ )
	{
		// numVerts
		sTokens.clear(); 
		getline(inFile, sLine); // strand 0 numVerts 25 texcoord 0.000000 0.522833
		StringTokenizer(sLine, string(" "), sTokens, false);
		iter = sTokens.begin() + 3;
		sToken = *(iter);
		int numVerts = atoi(sToken.c_str());

		for ( int vertex = 0; vertex < numVerts; vertex++ )
		{
			getline(inFile, sLine);
			sTokens.clear(); 
			int numFound = StringTokenizer(sLine, string(" "), sTokens, false);

			if ( numFound == 0 )
				continue;

			iter = sTokens.begin();

			CVector3D pnt;
			
			// x
			sToken = (*iter);			
			pnt.m_X = (btScalar)atof(sToken.c_str());

			// y
			++iter;
			sToken = (*iter);			
			pnt.m_Y = (btScalar)atof(sToken.c_str());

			// z
			++iter;
			sToken = (*iter);			
			pnt.m_Z = (btScalar)atof(sToken.c_str());

			CVertexHair vert;
			vert.m_Pos.Set(pnt.m_X, pnt.m_Y, pnt.m_Z);

			m_VertexArray.push_back(vert);
		}
		
	}

	inFile.close();


	// Set up indexes for vertices
	int index = 0;
	for ( std::vector<CVertexHair>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexHair& vert = *iter;
		vert.m_Index = index;
		index++;
	}
	
	FillSpringArray();
		
	return true;
}

void CHair::FillSpringArray()
{
	//---------------
	// Stretch springs
	//---------------
	m_StrechSpringArray.clear();

	for ( int i = 0; i < (int)m_VertexArray.size()-1; i++ )
	{
		CSpringHair edge;
		CVertexHair& vert = m_VertexArray[i];

		if ( i % 25 == 0 || i % 25 == 1 )
			vert.m_InvMass = 0;
		else
			vert.m_InvMass = 1.0f;
		
		edge.m_IndexVrx[0] = i;
		edge.m_IndexVrx[1] = i+1;
		edge.m_Index = i;
		m_StrechSpringArray.push_back(edge);
	}

	// Set rest length for stretch springs.
	for ( std::vector<CSpringHair>::iterator iterEdge = m_StrechSpringArray.begin(); iterEdge != m_StrechSpringArray.end(); iterEdge++ )
	{
		CSpringHair& edge = (*iterEdge);
		const CVector3D& ver0 = m_VertexArray[edge.GetVertexIndex(0)].m_Pos;
		const CVector3D& ver1 = m_VertexArray[edge.GetVertexIndex(1)].m_Pos;

		edge.SetRestLength((ver0 - ver1).Length());
	}

	//----------------
	// Bending springs
	//----------------	
	for ( std::vector<CSpringHair>::const_iterator iterEdge = m_StrechSpringArray.begin(); iterEdge != m_StrechSpringArray.end(); iterEdge++ )
	{
		const CSpringHair& edge = (*iterEdge);
		const CVector3D& ver0 = m_VertexArray[edge.GetVertexIndex(0)].m_Pos;
		const CVector3D& ver1 = m_VertexArray[edge.GetVertexIndex(1)].m_Pos;
	}		
	
	// Set rest length for bending springs.
	for ( std::vector<CSpringHair>::iterator iterEdge = m_BendSpringArray.begin(); iterEdge != m_BendSpringArray.end(); iterEdge++ )
	{
		CSpringHair& edge = (*iterEdge);
		const CVector3D& ver0 = m_VertexArray[edge.GetVertexIndex(0)].m_Pos;
		const CVector3D& ver1 = m_VertexArray[edge.GetVertexIndex(1)].m_Pos;

		edge.SetRestLength((ver0 - ver1).Length());
	}

	// Clear m_StrechSpringIndexes and m_BendSpringIndexes in each vertex
	for ( std::vector<CVertexHair>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexHair& vert = *iter;
		vert.m_StrechSpringIndexes.clear();
		vert.m_BendSpringIndexes.clear();
	}

	// Set m_StrechSpringIndexes in each vertex
	for ( std::vector<CSpringHair>::iterator iterEdge = m_StrechSpringArray.begin(); iterEdge != m_StrechSpringArray.end(); iterEdge++ )
	{
		CSpringHair& edge = (*iterEdge);
		CVertexHair& ver0 = m_VertexArray[edge.GetVertexIndex(0)];
		CVertexHair& ver1 = m_VertexArray[edge.GetVertexIndex(1)];

		ver0.m_StrechSpringIndexes.push_back(edge.GetIndex());
		ver1.m_StrechSpringIndexes.push_back(edge.GetIndex());
	}

	// Set m_BendSpringIndexes in each vertex
	for ( std::vector<CSpringHair>::iterator iterEdge = m_BendSpringArray.begin(); iterEdge != m_BendSpringArray.end(); iterEdge++ )
	{
		CSpringHair& edge = (*iterEdge);
		CVertexHair& ver0 = m_VertexArray[edge.GetVertexIndex(0)];
		CVertexHair& ver1 = m_VertexArray[edge.GetVertexIndex(1)];

		ver0.m_BendSpringIndexes.push_back(edge.GetIndex());
		ver1.m_BendSpringIndexes.push_back(edge.GetIndex());
	}
}

void CHair::SetGravity(const CVector3D& gravity)
{
	m_Gravity = gravity;
}

const CVector3D& CHair::GetGravity() const
{
	return m_Gravity;
}

void CHair::SetVertexMass(btScalar vertexMass)
{
	m_bEqualVertexMass = true;

	assert(vertexMass > 0 );

	btScalar invMass =  1.0f / vertexMass;

	for ( std::vector<CVertexHair>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexHair& vert = *iter;
		vert.m_InvMass = invMass;
	}
}

void CHair::SetTotalMass(btScalar totalMass)
{
	assert(totalMass > 0);

	m_bEqualVertexMass = true;

	btScalar invMass =  m_VertexArray.size() / totalMass;

	for ( std::vector<CVertexHair>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexHair& vert = *iter;
		vert.m_InvMass = invMass;
	}
}

void CHair::AddPin(int vertexIndex)
{
	if ( vertexIndex < 0 || vertexIndex >= (int)GetVertexArray().size() )
		return;

	
}

static bool ColoringCompare(const CSpringHair& a, const CSpringHair& b)
{
	return a.m_Coloring < b.m_Coloring;
}

void CHair::GenerateBatches()
{
	m_BatchSpringIndexArray.clear();

	for ( int i = 0; i < (int)m_StrechSpringArray.size(); i++ )
	{
		CSpringHair& spring = m_StrechSpringArray[i];

		const CVertexHair& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
		const CVertexHair& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

		int coloring = 0;		

		while ( true ) 
		{
			bool bFound0 = false;
			bool bFound1 = false;

			for ( int a = 0; a < (int)vert0.m_StrechSpringIndexes.size(); a++ )
			{
				const CSpringHair& otherSpring = m_StrechSpringArray[vert0.m_StrechSpringIndexes[a]];

				// skip if the neighbor spring is actually itself
				if ( otherSpring.GetIndex() == spring.GetIndex() )
					continue;

				if ( otherSpring.m_Coloring == coloring )
				{
					bFound0 = true;
					break;
				}				
			}
			
			for ( int a = 0; a < (int)vert1.m_StrechSpringIndexes.size(); a++ )
			{
				const CSpringHair& otherSpring = m_StrechSpringArray[vert1.m_StrechSpringIndexes[a]];

				// skip if the neighbor spring is actually itself
				if ( otherSpring.GetIndex() == spring.GetIndex() )
					continue;

				if ( otherSpring.m_Coloring == coloring )
				{
					bFound1 = true;
					break;
				}				
			}

			if ( bFound0 || bFound1 )
				coloring++;
			else
				break;
		} 

		spring.m_Coloring = coloring;
	}

#ifdef _DEBUG

	for ( int i = 0; i < (int)m_StrechSpringArray.size(); i++ )
	{
		CSpringHair& spring = m_StrechSpringArray[i];

		const CVertexHair& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
		const CVertexHair& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

		int coloring = spring.m_Coloring;
		bool bFound0 = false;
		bool bFound1 = false;

		for ( int a = 0; a < (int)vert0.m_StrechSpringIndexes.size(); a++ )
		{
			const CSpringHair& otherSpring = m_StrechSpringArray[vert0.m_StrechSpringIndexes[a]];

			// skip if the neighbor spring is actually itself
			if ( otherSpring.GetIndex() == spring.GetIndex() )
				continue;

			if ( otherSpring.m_Coloring == coloring )
			{
				bFound0 = true;
				break;
			}				
		}
		
		for ( int a = 0; a < (int)vert1.m_StrechSpringIndexes.size(); a++ )
		{
			const CSpringHair& otherSpring = m_StrechSpringArray[vert1.m_StrechSpringIndexes[a]];

			// skip if the neighbor spring is actually itself
			if ( otherSpring.GetIndex() == spring.GetIndex() )
				continue;

			if ( otherSpring.m_Coloring == coloring )
			{
				bFound1 = true;
				break;
			}				
		}

		assert(!bFound0 && !bFound1);
	}
#endif

	// Count how many batches were generated
	int countBatches = 0;

	for ( int i = 0; i < (int)m_StrechSpringArray.size(); i++ )
	{
		CSpringHair& spring = m_StrechSpringArray[i];

		if ( spring.m_Coloring > countBatches )
			countBatches = spring.m_Coloring;
	}

	countBatches++;

	std::sort(m_StrechSpringArray.begin(), m_StrechSpringArray.end(), ColoringCompare);

	m_BatchSpringIndexArray.push_back(0);

	if ( m_StrechSpringArray.size() > 1 )
	{
		int i = 0;

		for ( i = 0; i < (int)m_StrechSpringArray.size()-1; i++ )
		{
			CSpringHair& spring = m_StrechSpringArray[i];
			CSpringHair& springNext = m_StrechSpringArray[i+1];

#ifdef _DEBUG
			assert(spring.m_Coloring <= springNext.m_Coloring);
#endif

			if ( spring.m_Coloring < springNext.m_Coloring )
				m_BatchSpringIndexArray.push_back(i+1);
		}

		m_BatchSpringIndexArray.push_back(i);
	}

#ifdef _DEBUG
	for ( int i = 0; i < (int)m_BatchSpringIndexArray.size()-1; i++ )
	{
		int startIndex = m_BatchSpringIndexArray[i];
		int endIndex = m_BatchSpringIndexArray[i+1] - 1;

		for ( int j = startIndex; j <= endIndex; j++ )
		{
			assert(m_StrechSpringArray[j].m_Coloring == m_StrechSpringArray[startIndex].m_Coloring);
		}
	}
#endif

}

void CHair::Render()
{
	//----------
	// vertices
	//----------
	std::vector<CVertexHair>::const_iterator iter;
	
	glPointSize(4.0f);

	glBegin(GL_POINTS);
	for ( iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		const CVertexHair& vert = (*iter);
		

		glVertex3d(vert.m_Pos.m_X, vert.m_Pos.m_Y, vert.m_Pos.m_Z);
	}
	glEnd();

	glDisable(GL_LIGHTING);
	
	glColor3f(0, 0, 0);
	
	//----------------
	// Strech springs
	//----------------
	std::vector<CSpringHair>::const_iterator iterEdge;
	
	glColor3f(0.0f, 0.0f, 0.0f);
	glLineWidth(1.0);
	
	for ( iterEdge = m_StrechSpringArray.begin(); iterEdge != m_StrechSpringArray.end(); iterEdge++ )
	{
		const CSpringHair& edge = (*iterEdge);
		const CPoint3D& ver0 = m_VertexArray[edge.GetVertexIndex(0)].m_Pos;
		const CPoint3D& ver1 = m_VertexArray[edge.GetVertexIndex(1)].m_Pos;

		CPoint3D pos0 = (ver0);
		CPoint3D pos1 = (ver1);

		glBegin(GL_LINES);
		glVertex3d(pos0.m_X, pos0.m_Y, pos0.m_Z);
		glVertex3d(pos1.m_X, pos1.m_Y, pos1.m_Z);
		glEnd();
	}

	//--------------
	// Bend springs
	//--------------
	if ( 0 )
	{
		glColor3f(0.0f, 0.6f, 0.0f);
		glLineWidth(1.0);
		
		for ( iterEdge = m_BendSpringArray.begin(); iterEdge != m_BendSpringArray.end(); iterEdge++ )
		{
			const CSpringHair& edge = (*iterEdge);
			const CPoint3D& ver0 = m_VertexArray[edge.GetVertexIndex(0)].m_Pos;
			const CPoint3D& ver1 = m_VertexArray[edge.GetVertexIndex(1)].m_Pos;

			CPoint3D pos0 = (ver0);
			CPoint3D pos1 = (ver1);

			glBegin(GL_LINES);
			glVertex3d(pos0.m_X, pos0.m_Y, pos0.m_Z);
			glVertex3d(pos1.m_X, pos1.m_Y, pos1.m_Z);
			glEnd();
		}
	}
	
	glEnable(GL_LIGHTING);
}

int CHair::RenderBatch(int i) const
{
	glDisable(GL_LIGHTING);
	glLineWidth(3.0f);
	glColor3f(1.0f, 1.0f, 1.0f);

	if ( i >= (int)m_BatchSpringIndexArray.size() - 1 )
		i = i - ((int)m_BatchSpringIndexArray.size() - 1);

	if ( 0 <= i && i < (int)m_BatchSpringIndexArray.size() - 1  )
	{
		int startIndex = m_BatchSpringIndexArray[i];
		int endIndex = m_BatchSpringIndexArray[i+1] - 1;

		for ( int j = startIndex; j <= endIndex; j++ )
		{
			const CVector3D& v0 = m_VertexArray[m_StrechSpringArray[j].GetVertexIndex(0)].m_Pos;
			const CVector3D& v1 = m_VertexArray[m_StrechSpringArray[j].GetVertexIndex(1)].m_Pos;

			glBegin(GL_LINES);
			glVertex3f(v0.m_X, v0.m_Y, v0.m_Z);
			glVertex3f(v1.m_X, v1.m_Y, v1.m_Z);
			glEnd();
		}
	}

	glEnable(GL_LIGHTING);

	return i;
}

void CHair::ApplyForces(btScalar dt)
{
	#pragma omp parallel for
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexHair& vert = m_VertexArray[i];		
		vert.m_Vel += vert.m_Accel * dt;
	}
}

void CHair::ApplyGravity(btScalar dt)
{
	#pragma omp parallel for
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexHair& vert = m_VertexArray[i];	

		if ( vert.m_InvMass > 0 )
			vert.m_Accel += m_Gravity;
	}
}

void CHair::ClearForces()
{
	#pragma omp parallel for
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexHair& vert = m_VertexArray[i];		
		vert.m_Accel.Set(0, 0, 0);
	}
}

void CHair::ComputeNextVertexPositions(btScalar dt)
{
	#pragma omp parallel for
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexHair& vert = m_VertexArray[i];		

		vert.m_PosNext = vert.m_Pos + vert.m_Vel * dt;
	}
}

void CHair::EnforceEdgeConstraints(btScalar dt)
{
	m_dt = dt;
	
	for ( int i = 0; i < (int)m_StrechSpringArray.size(); i++ )
	{
		int indexEdge = i;

		bool bNeedLimiting = false;

		const CSpringHair& spring = m_StrechSpringArray[indexEdge];

		CVertexHair& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
		CVertexHair& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

		CVector3D vecNewSpring = vert0.m_PosNext - vert1.m_PosNext;

		btScalar newLen = vecNewSpring.Length();
		btScalar restLen = spring.GetRestLength();

		CVector3D cji = (newLen-restLen)*vecNewSpring.Normalize();

		CVector3D dVert0(0, 0, 0);
		CVector3D dVert1(0, 0, 0);			

		if ( vert0.m_InvMass == 0 && vert1.m_InvMass > 0 ) // if vert0 is pinned and vert1 is not
		{
			dVert1 = cji;
		}
		else if ( vert0.m_InvMass > 0 && vert1.m_InvMass == 0 ) // if vert1 is pinned and vert0 is not
		{
			dVert0 = -cji;
		}
		else if ( vert0.m_InvMass > 0 && vert1.m_InvMass > 0 ) // if both are not pinned
		{
			dVert0 = -0.5*cji;
			dVert1 = 0.5*cji;
		}

		vert0.m_PosNext += dVert0;
		vert1.m_PosNext += dVert1;			  
	}	
}

void CHair::EnforceEdgeConstraintsBatched(btScalar dt)
{
	m_dt = dt;
	
	#pragma omp parallel for
	for ( int batch = 0; batch < (int)m_BatchSpringIndexArray.size()-1; batch++ )
	{
		int startIndex = m_BatchSpringIndexArray[batch];
		int endIndex = m_BatchSpringIndexArray[batch+1] - 1;

		for ( int j = startIndex; j <= endIndex; j++ )
		{
			int indexEdge = j;

			bool bNeedLimiting = false;

			const CSpringHair& spring = m_StrechSpringArray[indexEdge];

			CVertexHair& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
			CVertexHair& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

			CVector3D vecNewSpring = vert0.m_PosNext - vert1.m_PosNext;

			btScalar newLen = vecNewSpring.Length();
			btScalar restLen = spring.GetRestLength();

			CVector3D cji = (newLen-restLen)*vecNewSpring.Normalize();

			CVector3D dVert0(0, 0, 0);
			CVector3D dVert1(0, 0, 0);			

			if ( vert0.m_InvMass == 0 && vert1.m_InvMass > 0 ) // if vert0 is pinned and vert1 is not
			{
				dVert1 = cji;
			}
			else if ( vert0.m_InvMass > 0 && vert1.m_InvMass == 0 ) // if vert1 is pinned and vert0 is not
			{
				dVert0 = -cji;
			}
			else if ( vert0.m_InvMass > 0 && vert1.m_InvMass > 0 ) // if both are not pinned
			{
				dVert0 = -0.5*cji;
				dVert1 = 0.5*cji;
			}

			vert0.m_PosNext += dVert0;
			vert1.m_PosNext += dVert1;			  
		}
	}	
}

bool CHair::AdvancePosition(btScalar dt)
{
	m_dt = dt;

	#pragma omp parallel for
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexHair& vert = m_VertexArray[i];	

		if ( vert.m_InvMass > 0 )
			vert.m_Pos = vert.m_Pos + vert.m_Vel * dt;
	}

	return true;
}

bool CHair::Integrate(btScalar dt)
{
	m_dt = dt;

	ClearForces();
	ApplyGravity(dt);

	// apply bending spring forces
	std::vector<CSpringHair>::const_iterator iterBendSpring;

	for ( iterBendSpring = m_BendSpringArray.begin(); iterBendSpring != m_BendSpringArray.end(); iterBendSpring++ )
	{
		const CSpringHair& spring = (*iterBendSpring);

		CVertexHair& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
		CVertexHair& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

		CVector3D vec = (vert1.m_Pos - vert0.m_Pos);
		btScalar len = vec.Length();
		CVector3D springForce = m_Kb * (len - spring.GetRestLength()) * vec.Normalize();

		if ( vert0.m_InvMass > 0 )
			vert0.m_Accel += springForce * vert0.m_InvMass;

		if ( vert1.m_InvMass > 0 )
			vert1.m_Accel += -springForce * vert1.m_InvMass;
	}

	ApplyForces(dt);
	ClearForces();
	ComputeNextVertexPositions(dt);

	int numIteration = 0;
	bool bNeedMoreIteration = true;
	
	while ( bNeedMoreIteration && numIteration < m_NumIterForConstraintSolver )
	{
		EnforceEdgeConstraints(dt);		
		//EnforceEdgeConstraintsBatched(dt);
		++numIteration;
	}

	UpdateVelocities(dt);

	m_NumIter = numIteration;
	return true;
}

bool CHair::ResolveCollision(CCollisionObject& convexObject, btScalar dt)
{
	CNarrowPhaseCollisionDetection np;

	for ( int i = 0; i < (int)GetVertexArray().size(); i++ )
	{
		CNarrowCollisionInfo info;
		CVertexHair& vert = GetVertexArray()[i];
		
		CCollisionObject pointColObj;
		pointColObj.SetCollisionObjectType(CCollisionObject::Point);

		// We use the next position of vertices which might become the current positions if there is no collision at the end of step. 
		pointColObj.GetTransform().GetTranslation() = vert.m_PosNext;

		if ( np.GetConvexCollisionAlgorithm()->CheckCollision(convexObject, pointColObj, &info, false) )
		{
			CVector3D pointAW = convexObject.GetTransform() * info.witnessPntA;
			CVector3D pointBW = pointColObj.GetTransform().GetTranslation(); // it is a point object. 
			CVector3D v = pointAW - pointBW;
			CVector3D n = v.NormalizeOther();
			btScalar d = info.penetrationDepth; // d already contains margin
			//btScalar margin = convexObject.GetMargin();

			// TODO:Need to know translational and angular velocities of object A.
			CVector3D velOnPointAW(0, 0, 0);
			
			// critical relative velocity to separate the vertex and object
			btScalar critical_relVel = d / dt;

			CVector3D relVel = vert.m_Vel-velOnPointAW;

			// relative normal velocity of vertex. If positive, vertex is separating from the object.
			btScalar relVelNLen = relVel.Dot(n);
			CVector3D relVelN = relVelNLen * n;

			// relative tangential velocity to calculate friction
			CVector3D relVelT = relVel - relVelN;

			// friction.
			btScalar mu = GetFrictionCoef();
			CVector3D dVT(0, 0, 0);

			btScalar relVelTLen = relVelT.Length();

			if ( mu > 0 && relVelTLen > 0 )
			{
				CVector3D newVelT = max(1.0f-mu*relVelNLen/relVelTLen, 0) * relVelT;
				dVT = -(newVelT - relVelT); 
			}

			CVector3D dVN = (critical_relVel) * n;
			CVector3D dV = dVN + dVT;

			if ( vert.m_InvMass > 0 )
			{
				vert.m_Vel += dV;
			}
		}
	}

	return true;
}

void CHair::UpdateVelocities(btScalar dt)
{
	for ( unsigned int i = 0; i < m_VertexArray.size(); i++ )
	{
		CVertexHair& vert = m_VertexArray[i];

		if ( vert.m_InvMass > 0 )
			vert.m_Vel = (vert.m_PosNext - vert.m_Pos)/dt;
	}
}

void CHair::TranslateW(btScalar x, btScalar y, btScalar z)
{
	for ( std::vector<CVertexHair>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexHair& vert = *iter;
				
		vert.m_Pos += CVector3D(x, y, z);
	}
}

CHair& CHair::operator=(const CHair& other) 
{ 
	m_Kst = other.m_Kst;
	m_Kb = other.m_Kb;
	m_Kd = other.m_Kd;

	// ToDo: Need to do something with m_pBVHTree

	m_VertexArray = other.m_VertexArray;
	m_StrechSpringArray = other.m_StrechSpringArray;
	m_BendSpringArray = other.m_BendSpringArray;

	m_bDeformable = other.m_bDeformable;
	m_bEqualVertexMass = other.m_bEqualVertexMass;

	return *this; 
}




