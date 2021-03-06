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

#include "Cloth.h"
#include "StringTokenizer.h"
#include "NarrowPhaseCollisionDetection.h"
#include "ConvexCollisionAlgorithm.h"
#include "CollisionObject.h"

using namespace std;


CCloth::CCloth(void) : m_bDeformable(true), m_Gravity(0.0f, -9.8f, 0.0f), m_dt(0.0f), m_bShowBV(false)
{
	m_Kst = 10000.0f;
	m_Ksh = 10000.0f;
	m_Kb = 5000.0f;
	m_Kd = 0.0f;
	m_Mu = 0.3f;
	
	m_bEqualVertexMass = true;
	m_NumIterForConstraintSolver = 7;
}

CCloth::CCloth(const CCloth& other)
{
	m_Kst = other.m_Kst;
	m_Ksh = other.m_Ksh;
	m_Kb = other.m_Kb;
	m_Kd = other.m_Kd;
	m_Gravity = other.m_Gravity;

	m_VertexArray = other.m_VertexArray;
	m_StrechSpringArray = other.m_StrechSpringArray;
	m_BendSpringArray = other.m_BendSpringArray;
	m_Color = other.m_Color;

	m_bDeformable = other.m_bDeformable;
	m_bEqualVertexMass = other.m_bEqualVertexMass;
	m_bShowBV = other.m_bShowBV;
}

CCloth::~CCloth(void)
{
	Clear();
}

void CCloth::Clear()
{
	m_VertexArray.clear();
	m_StrechSpringArray.clear();
	m_BendSpringArray.clear();

	m_Color = COLOR();
}

void CCloth::Initialize()
{
	m_bDeformable = true;

	GenerateBatches();
}

bool CCloth::Load(const char* filename)
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
			CPoint3D pnt;
			
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

			CVertexCloth3D vert;
			vert.m_Pos.Set(pnt.m_X, pnt.m_Y, pnt.m_Z);

			m_VertexArray.push_back(vert);
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

			//m_NormalVecArray.push_back(n);
		}
		else if ( sToken == "f" ) // face
		{
			CTriangleCloth3D tri;
			vector<string> sTokens2;

			int i = 0;

			for ( iter = sTokens.begin() + 1; iter != sTokens.end(); iter++ )
			{
				sToken = (*iter);
				sTokens2.clear();
				numFound = StringTokenizer(sToken, string("/"), sTokens2, false);

				if ( numFound > 0 )
				{
					tri.m_IndexVrx[i++] = atoi(sTokens2[0].c_str())-1;

					//if ( numFound == 3 )
					//	tri.m_IndexNormalVec = atoi(sTokens2[2].c_str());
				}
				else if ( numFound == 0 && sToken != "" )
				{
					tri.m_IndexVrx[i++] = atoi(sToken.c_str())-1;
				}
			}		

			tri.m_Index = (int)m_TriangleArray.size();
			m_TriangleArray.push_back(tri);	
		}		
	}

	inFile.close();

	m_NormalVecArray.resize(m_VertexArray.size());

	// Set up indexes for vertices
	int index = 0;
	for ( std::vector<CVertexCloth3D>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexCloth3D& vert = *iter;
		vert.m_Index = index;
		index++;
	}
	
	FillSpringArray();
		
	return true;
}

void CCloth::FillSpringArray()
{
	//---------------
	// Stretch springs
	//---------------
	m_StrechSpringArray.clear();

	std::vector<CTriangleCloth3D>::iterator iterTri;

	for ( iterTri = m_TriangleArray.begin(); iterTri != m_TriangleArray.end(); ++iterTri )
	{
		const CTriangleCloth3D& tri = (*iterTri);

		for ( int i = 0; i < 3; i++ )
		{
			int j = ((i != 2) ? i+1 : 0);

			CSpringCloth3D edge(tri.GetVertexIndex(i), tri.GetVertexIndex(j));	
			vector<CSpringCloth3D>::iterator iterEdge = std::find(m_StrechSpringArray.begin(), m_StrechSpringArray.end(), edge);

			if ( iterEdge == m_StrechSpringArray.end() )
			{
				edge.m_IndexTriangle[0] = tri.GetIndex();
				edge.m_IndexVrx[0] = tri.GetVertexIndex(i);
				edge.m_IndexVrx[1] = tri.GetVertexIndex(j);

				edge.m_Index = m_StrechSpringArray.size();
				m_StrechSpringArray.push_back(edge);
			}
			else
				(*iterEdge).m_IndexTriangle[1] = tri.GetIndex();
		}		
	}

	for ( iterTri = m_TriangleArray.begin(); iterTri != m_TriangleArray.end(); ++iterTri )
	{
		CTriangleCloth3D& tri = (*iterTri);

		for ( int i = 0; i < 3; i++ )
		{
			int j = ((i != 2) ? i+1 : 0);

			CSpringCloth3D edge(tri.GetVertexIndex(i), tri.GetVertexIndex(j));	
			vector<CSpringCloth3D>::iterator iterEdge = std::find(m_StrechSpringArray.begin(), m_StrechSpringArray.end(), edge);

			if ( iterEdge == m_StrechSpringArray.end() )
				assert(0); // must not reach here!

			tri.m_IndexEdge[i] = (*iterEdge).GetIndex();
		}		
	}

	// Set rest length for stretch springs.
	for ( std::vector<CSpringCloth3D>::iterator iterEdge = m_StrechSpringArray.begin(); iterEdge != m_StrechSpringArray.end(); iterEdge++ )
	{
		CSpringCloth3D& edge = (*iterEdge);
		const CVector3D& ver0 = m_VertexArray[edge.GetVertexIndex(0)].m_Pos;
		const CVector3D& ver1 = m_VertexArray[edge.GetVertexIndex(1)].m_Pos;

		edge.SetRestLength((ver0 - ver1).Length());
	}

	//----------------
	// Bending springs
	//----------------	
	for ( std::vector<CSpringCloth3D>::const_iterator iterEdge = m_StrechSpringArray.begin(); iterEdge != m_StrechSpringArray.end(); iterEdge++ )
	{
		const CSpringCloth3D& edge = (*iterEdge);
		const CVector3D& ver0 = m_VertexArray[edge.GetVertexIndex(0)].m_Pos;
		const CVector3D& ver1 = m_VertexArray[edge.GetVertexIndex(1)].m_Pos;

		int indexTri0 = edge.m_IndexTriangle[0];
		int indexTri1 = edge.m_IndexTriangle[1];

		if ( indexTri0 != -1 && indexTri1 != -1 )
		{
			const CTriangleCloth3D& tri0 = m_TriangleArray[indexTri0];
			const CTriangleCloth3D& tri1 = m_TriangleArray[indexTri1];

			int indexVer0 = -1;
			int indexVer1 = -1;

			// find two vertices whose connecting line crosses the current edge.
			if ( tri0.GetVertexIndex(0) != edge.GetVertexIndex(0) && tri0.GetVertexIndex(0) != edge.GetVertexIndex(1) )
				indexVer0 = tri0.GetVertexIndex(0);
			else if ( tri0.GetVertexIndex(1) != edge.GetVertexIndex(0) && tri0.GetVertexIndex(1) != edge.GetVertexIndex(1) )
				indexVer0 = tri0.GetVertexIndex(1);
			else if ( tri0.GetVertexIndex(2) != edge.GetVertexIndex(0) && tri0.GetVertexIndex(2) != edge.GetVertexIndex(1) )
				indexVer0 = tri0.GetVertexIndex(2);

			if ( tri1.GetVertexIndex(0) != edge.GetVertexIndex(0) && tri1.GetVertexIndex(0) != edge.GetVertexIndex(1) )
				indexVer1 = tri1.GetVertexIndex(0);
			else if ( tri1.GetVertexIndex(1) != edge.GetVertexIndex(0) && tri1.GetVertexIndex(1) != edge.GetVertexIndex(1) )
				indexVer1 = tri1.GetVertexIndex(1);
			else if ( tri1.GetVertexIndex(2) != edge.GetVertexIndex(0) && tri1.GetVertexIndex(2) != edge.GetVertexIndex(1) )
				indexVer1 = tri1.GetVertexIndex(2);

			assert(indexVer0 != -1 && indexVer1 != -1);

			CSpringCloth3D bendSpring(indexVer0, indexVer1);

			vector<CSpringCloth3D>::iterator iterCheck = std::find(m_BendSpringArray.begin(), m_BendSpringArray.end(), bendSpring);

			// there is already the same edge in the array.
			if ( iterCheck != m_BendSpringArray.end() )
				continue;

			bendSpring.m_Index = m_BendSpringArray.size();
			m_BendSpringArray.push_back(bendSpring);
		}		
	}		
	
	// Set rest length for bending springs.
	for ( std::vector<CSpringCloth3D>::iterator iterEdge = m_BendSpringArray.begin(); iterEdge != m_BendSpringArray.end(); iterEdge++ )
	{
		CSpringCloth3D& edge = (*iterEdge);
		const CVector3D& ver0 = m_VertexArray[edge.GetVertexIndex(0)].m_Pos;
		const CVector3D& ver1 = m_VertexArray[edge.GetVertexIndex(1)].m_Pos;

		edge.SetRestLength((ver0 - ver1).Length());
	}

	// Clear m_StrechSpringIndexes and m_BendSpringIndexes in each vertex
	for ( std::vector<CVertexCloth3D>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexCloth3D& vert = *iter;
		vert.m_StrechSpringIndexes.clear();
		vert.m_BendSpringIndexes.clear();
	}

	// Set m_StrechSpringIndexes in each vertex
	for ( std::vector<CSpringCloth3D>::iterator iterEdge = m_StrechSpringArray.begin(); iterEdge != m_StrechSpringArray.end(); iterEdge++ )
	{
		CSpringCloth3D& edge = (*iterEdge);
		CVertexCloth3D& ver0 = m_VertexArray[edge.GetVertexIndex(0)];
		CVertexCloth3D& ver1 = m_VertexArray[edge.GetVertexIndex(1)];

		ver0.m_StrechSpringIndexes.push_back(edge.GetIndex());
		ver1.m_StrechSpringIndexes.push_back(edge.GetIndex());
	}

	// Set m_BendSpringIndexes in each vertex
	for ( std::vector<CSpringCloth3D>::iterator iterEdge = m_BendSpringArray.begin(); iterEdge != m_BendSpringArray.end(); iterEdge++ )
	{
		CSpringCloth3D& edge = (*iterEdge);
		CVertexCloth3D& ver0 = m_VertexArray[edge.GetVertexIndex(0)];
		CVertexCloth3D& ver1 = m_VertexArray[edge.GetVertexIndex(1)];

		ver0.m_BendSpringIndexes.push_back(edge.GetIndex());
		ver1.m_BendSpringIndexes.push_back(edge.GetIndex());
	}
}

void CCloth::SetGravity(const CVector3D& gravity)
{
	m_Gravity = gravity;
}

const CVector3D& CCloth::GetGravity() const
{
	return m_Gravity;
}

void CCloth::SetVertexMass(float vertexMass)
{
	m_bEqualVertexMass = true;

	assert(vertexMass > 0 );

	float invMass =  1.0f / vertexMass;

	for ( std::vector<CVertexCloth3D>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexCloth3D& vert = *iter;
		vert.m_InvMass = invMass;
	}
}

void CCloth::SetTotalMass(float totalMass)
{
	assert(totalMass > 0);

	m_bEqualVertexMass = true;

	float invMass =  m_VertexArray.size() / totalMass;

	for ( std::vector<CVertexCloth3D>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexCloth3D& vert = *iter;
		vert.m_InvMass = invMass;
	}
}

void CCloth::AddPin(int vertexIndex)
{
	if ( vertexIndex < 0 || vertexIndex >= (int)GetVertexArray().size() )
		return;

	CVertexCloth3D* pVertex = &GetVertexArray()[vertexIndex];
	CClothPin pin(pVertex, pVertex->m_Pos);
	m_PinArray.push_back(pin);

	pVertex->m_pPin = &m_PinArray[m_PinArray.size()-1];
	pVertex->m_PinIndex = m_PinArray.size()-1;
}

static bool ColoringCompare(const CSpringCloth3D& a, const CSpringCloth3D& b)
{
	return a.m_Coloring < b.m_Coloring;
}

void CCloth::GenerateBatches()
{
	m_BatchSpringIndexArray.clear();

	for ( int i = 0; i < (int)m_StrechSpringArray.size(); i++ )
	{
		CSpringCloth3D& spring = m_StrechSpringArray[i];

		const CVertexCloth3D& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
		const CVertexCloth3D& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

		int coloring = 0;		

		while ( true ) 
		{
			bool bFound0 = false;
			bool bFound1 = false;

			for ( int a = 0; a < (int)vert0.m_StrechSpringIndexes.size(); a++ )
			{
				const CSpringCloth3D& otherSpring = m_StrechSpringArray[vert0.m_StrechSpringIndexes[a]];

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
				const CSpringCloth3D& otherSpring = m_StrechSpringArray[vert1.m_StrechSpringIndexes[a]];

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
		CSpringCloth3D& spring = m_StrechSpringArray[i];

		const CVertexCloth3D& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
		const CVertexCloth3D& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

		int coloring = spring.m_Coloring;
		bool bFound0 = false;
		bool bFound1 = false;

		for ( int a = 0; a < (int)vert0.m_StrechSpringIndexes.size(); a++ )
		{
			const CSpringCloth3D& otherSpring = m_StrechSpringArray[vert0.m_StrechSpringIndexes[a]];

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
			const CSpringCloth3D& otherSpring = m_StrechSpringArray[vert1.m_StrechSpringIndexes[a]];

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
		CSpringCloth3D& spring = m_StrechSpringArray[i];

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
			CSpringCloth3D& spring = m_StrechSpringArray[i];
			CSpringCloth3D& springNext = m_StrechSpringArray[i+1];

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

void CCloth::Render()
{
	// Calculate normal vectors for each vertex
	for ( std::vector<CVector3D>::iterator iter = m_NormalVecArray.begin(); iter != m_NormalVecArray.end(); iter++ )
	{
		(*iter).Set(0, 0, 0);
	}

	for ( std::vector<CTriangleCloth3D>::const_iterator iterTri = m_TriangleArray.begin(); iterTri != m_TriangleArray.end(); iterTri++ )
	{
		const CTriangleCloth3D& tri = (*iterTri);	

		const CVertexCloth3D& v0 = m_VertexArray[tri.GetVertexIndex(0)];
		const CVertexCloth3D& v1 = m_VertexArray[tri.GetVertexIndex(1)];
		const CVertexCloth3D& v2 = m_VertexArray[tri.GetVertexIndex(2)];
		
		CVector3D n = CVector3D(v0.m_Pos, v1.m_Pos).Cross(CVector3D(v0.m_Pos, v2.m_Pos)).Normalize();

		int idx0 = v0.GetIndex();
		int idx1 = v1.GetIndex();
		int idx2 = v2.GetIndex();

		m_NormalVecArray[idx0] += n;
		m_NormalVecArray[idx1] += n;
		m_NormalVecArray[idx2] += n;
	}

	//-----------
	// triangles
	//-----------
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	GLfloat white[] = {1.0f, 1.0f, 1.0f, 1.0f};
	GLfloat color[]  = { 0.0f, 0.0f, 0.0f, 1.0f};
	color[0] = m_Color.r;
	color[1] = m_Color.g;
	color[2] = m_Color.b;

	glMaterialfv(GL_FRONT, GL_DIFFUSE, color);
	
	glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 1.0);

	glBegin(GL_TRIANGLES);

	for ( std::vector<CTriangleCloth3D>::const_iterator iterTri = m_TriangleArray.begin(); iterTri != m_TriangleArray.end(); iterTri++ )
	{
		const CTriangleCloth3D& tri = (*iterTri);	

		for ( int i = 0; i < 3; i++ )
		{
			const CVertexCloth3D& v = m_VertexArray[tri.GetVertexIndex(i)];

			const CVector3D& n = m_NormalVecArray[v.GetIndex()];
			glNormal3d(n.m_X, n.m_Y, n.m_Z);
			glVertex3d(v.m_Pos.m_X, v.m_Pos.m_Y, v.m_Pos.m_Z);
		}

	}	

	glEnd();

	/*color[0] = 0.3f;
	color[1] = 0.5f;
	color[2] = 0.1f;*/
	color[0] = 0.3f;
	color[1] = 0.3f;
	color[2] = 0.3f;
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);

	glBegin(GL_TRIANGLES);

	for ( std::vector<CTriangleCloth3D>::const_iterator iterTri = m_TriangleArray.begin(); iterTri != m_TriangleArray.end(); iterTri++ )
	{
		const CTriangleCloth3D& tri = (*iterTri);	

		const CVertexCloth3D& v0 = m_VertexArray[tri.GetVertexIndex(0)];
		const CVertexCloth3D& v1 = m_VertexArray[tri.GetVertexIndex(1)];
		const CVertexCloth3D& v2 = m_VertexArray[tri.GetVertexIndex(2)];
		
		CVector3D n = CVector3D(v0.m_Pos, v1.m_Pos).Cross(CVector3D(v1.m_Pos, v2.m_Pos)).Normalize();

		glNormal3d(-n.m_X, -n.m_Y, -n.m_Z);
		glVertex3d(v2.m_Pos.m_X, v2.m_Pos.m_Y, v2.m_Pos.m_Z);
		glVertex3d(v1.m_Pos.m_X, v1.m_Pos.m_Y, v1.m_Pos.m_Z);
		glVertex3d(v0.m_Pos.m_X, v0.m_Pos.m_Y, v0.m_Pos.m_Z);
	}	

	glEnd();

	glDisable(GL_POLYGON_OFFSET_FILL);

	//----------
	// vertices
	//----------
	std::vector<CVertexCloth3D>::const_iterator iter;

	glDisable(GL_LIGHTING);
	
	//glColor3f(m_Color.r, m_Color.g, m_Color.b);
	glColor3f(0, 0, 0);
	
	//----------------
	// Strech springs
	//----------------
	std::vector<CSpringCloth3D>::const_iterator iterEdge;
	
	glColor3f(0.0f, 0.0f, 0.0f);
	glLineWidth(1.0);
	
	for ( iterEdge = m_StrechSpringArray.begin(); iterEdge != m_StrechSpringArray.end(); iterEdge++ )
	{
		const CSpringCloth3D& edge = (*iterEdge);
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
			const CSpringCloth3D& edge = (*iterEdge);
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

int CCloth::RenderBatch(int i) const
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

void CCloth::ApplyForces(float dt)
{
	#pragma omp parallel for
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];		
		vert.m_Vel += vert.m_Accel * dt;
	}
}

void CCloth::ApplyGravity(float dt)
{
	#pragma omp parallel for
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];	

		if ( !vert.m_pPin )
			vert.m_Accel += m_Gravity;
	}
}

void CCloth::ClearForces()
{
	#pragma omp parallel for
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];		
		vert.m_Accel.Set(0, 0, 0);
	}
}

void CCloth::ComputeNextVertexPositions(float dt)
{
	#pragma omp parallel for
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];		

		if ( !vert.m_pPin )
			vert.m_PosNext = vert.m_Pos + vert.m_Vel * dt;
		else
			vert.m_PosNext = vert.m_Pos;
	}
}

void CCloth::EnforceEdgeConstraints(float dt)
{
	m_dt = dt;
	
	for ( int i = 0; i < (int)m_StrechSpringArray.size(); i++ )
	{
		int indexEdge = i;

		bool bNeedLimiting = false;

		const CSpringCloth3D& spring = m_StrechSpringArray[indexEdge];

		CVertexCloth3D& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
		CVertexCloth3D& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

		CVector3D vecNewSpring = vert0.m_PosNext - vert1.m_PosNext;

		float newLen = vecNewSpring.Length();
		float restLen = spring.GetRestLength();

		CVector3D cji = (newLen-restLen)*vecNewSpring.Normalize();

		CVector3D dVert0(0, 0, 0);
		CVector3D dVert1(0, 0, 0);			

		if ( vert0.m_pPin && !vert1.m_pPin ) // if vert0 is pinned and vert1 is not
		{
			dVert1 = cji;
		}
		else if ( !vert0.m_pPin && vert1.m_pPin ) // if vert1 is pinned and vert0 is not
		{
			dVert0 = -cji;
		}
		else if ( !vert0.m_pPin && !vert1.m_pPin ) // if both are not pinned
		{
			dVert0 = -0.5*cji;
			dVert1 = 0.5*cji;
		}

		vert0.m_PosNext += dVert0;
		vert1.m_PosNext += dVert1;			  
	}	
}

void CCloth::EnforceEdgeConstraintsBatched(float dt)
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

			const CSpringCloth3D& spring = m_StrechSpringArray[indexEdge];

			CVertexCloth3D& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
			CVertexCloth3D& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

			CVector3D vecNewSpring = vert0.m_PosNext - vert1.m_PosNext;

			float newLen = vecNewSpring.Length();
			float restLen = spring.GetRestLength();

			CVector3D cji = (newLen-restLen)*vecNewSpring.Normalize();

			CVector3D dVert0(0, 0, 0);
			CVector3D dVert1(0, 0, 0);			

			if ( vert0.m_pPin && !vert1.m_pPin ) // if vert0 is pinned and vert1 is not
			{
				dVert1 = cji;
			}
			else if ( !vert0.m_pPin && vert1.m_pPin ) // if vert1 is pinned and vert0 is not
			{
				dVert0 = -cji;
			}
			else if ( !vert0.m_pPin && !vert1.m_pPin ) // if both are not pinned
			{
				dVert0 = -0.5*cji;
				dVert1 = 0.5*cji;
			}

			vert0.m_PosNext += dVert0;
			vert1.m_PosNext += dVert1;			  
		}
	}	
}

bool CCloth::AdvancePosition(float dt)
{
	m_dt = dt;

	#pragma omp parallel for
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];	

		if ( vert.m_pPin )
			vert.m_Pos = vert.m_pPin->GetPinPos();
		else
			vert.m_Pos = vert.m_Pos + vert.m_Vel * dt;
	}

	return true;
}

bool CCloth::Integrate(float dt)
{
	m_dt = dt;

	ClearForces();
	ApplyGravity(dt);

	// apply bending spring forces
	std::vector<CSpringCloth3D>::const_iterator iterBendSpring;

	for ( iterBendSpring = m_BendSpringArray.begin(); iterBendSpring != m_BendSpringArray.end(); iterBendSpring++ )
	{
		const CSpringCloth3D& spring = (*iterBendSpring);

		CVertexCloth3D& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
		CVertexCloth3D& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

		CVector3D vec = (vert1.m_Pos - vert0.m_Pos);
		float len = vec.Length();
		CVector3D springForce = m_Kb * (len - spring.GetRestLength()) * vec.Normalize();

		if ( !vert0.m_pPin )
			vert0.m_Accel += springForce * vert0.m_InvMass;

		if ( !vert1.m_pPin )
			vert1.m_Accel += -springForce * vert1.m_InvMass;
	}

	ApplyForces(dt);
	ClearForces();
	ComputeNextVertexPositions(dt);

	int numIteration = 0;
	bool bNeedMoreIteration = true;
	
	while ( bNeedMoreIteration && numIteration < m_NumIterForConstraintSolver )
	{
		//EnforceEdgeConstraints(dt);		
		EnforceEdgeConstraintsBatched(dt);
		++numIteration;
	}

	UpdateVelocities(dt);

	m_NumIter = numIteration;
	return true;
}

bool CCloth::ResolveCollision(CCollisionObject& convexObject, float dt)
{
	CNarrowPhaseCollisionDetection np;

	for ( int i = 0; i < (int)GetVertexArray().size(); i++ )
	{
		CNarrowCollisionInfo info;
		CVertexCloth3D& vert = GetVertexArray()[i];
		
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
			float d = info.penetrationDepth; // d already contains margin
			//float margin = convexObject.GetMargin();

			// TODO:Need to know translational and angular velocities of object A.
			CVector3D velOnPointAW(0, 0, 0);
			
			// critical relative velocity to separate the vertex and object
			float critical_relVel = d / dt;

			CVector3D relVel = vert.m_Vel-velOnPointAW;

			// relative normal velocity of vertex. If positive, vertex is separating from the object.
			float relVelNLen = relVel.Dot(n);
			CVector3D relVelN = relVelNLen * n;

			// relative tangential velocity to calculate friction
			CVector3D relVelT = relVel - relVelN;

			// friction.
			float mu = GetFrictionCoef();
			CVector3D dVT(0, 0, 0);

			float relVelTLen = relVelT.Length();

			if ( mu > 0 && relVelTLen > 0 )
			{
				CVector3D newVelT = max(1.0-mu*relVelNLen/relVelTLen, 0) * relVelT;
				dVT = -(newVelT - relVelT); 
			}

			CVector3D dVN = (critical_relVel) * n;
			CVector3D dV = dVN + dVT;

			if ( !vert.m_pPin )
			{
				vert.m_Vel += dV;
			}
		}
	}

	return true;
}

void CCloth::UpdateVelocities(float dt)
{
	for ( unsigned int i = 0; i < m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];

		if ( !vert.m_pPin )
			vert.m_Vel = (vert.m_PosNext - vert.m_Pos)/dt;
	}
}

void CCloth::TranslateW(float x, float y, float z)
{
	for ( std::vector<CVertexCloth3D>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexCloth3D& vert = *iter;
				
		vert.m_Pos += CVector3D(x, y, z);
	}
}

CCloth& CCloth::operator=(const CCloth& other) 
{ 
	m_Kst = other.m_Kst;
	m_Kb = other.m_Kb;
	m_Kd = other.m_Kd;

	// ToDo: Need to do something with m_pBVHTree

	m_VertexArray = other.m_VertexArray;
	m_StrechSpringArray = other.m_StrechSpringArray;
	m_BendSpringArray = other.m_BendSpringArray;
	m_Color = other.m_Color;

	m_bDeformable = other.m_bDeformable;
	m_bEqualVertexMass = other.m_bEqualVertexMass;
	m_bShowBV = other.m_bShowBV;

	return *this; 
}




