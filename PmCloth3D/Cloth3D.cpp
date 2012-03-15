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

#include "Cloth3D.h"
#include "BVHTree.h"
#include "BVHMasterTree.h"
#include "StringTokenizer.h"
#include "MINRESSolver.h"

using namespace std;


CCloth3D::CCloth3D(void) : m_pBVHTree(NULL), m_bDeformable(true), m_Gravity(0, 0, -5.0), m_dt(0), m_bShowBV(false)
{
	// If thickness is too large (> 0.1), the solver seems to get slower and it will eventually halt if the thickness is 
	// greater than 1.0 or something. 
	// I don't know what causes it. Also friction doesn't work well if the thickness is large like greater than 0.05. 
	// 0.01 seems to be the best number. 
	// TODO:need to find out why the solver becomes slow and friction doesn't work if thickness is large.
	m_h = 0.01;
	m_Kst = 10000.0;
	m_Ksh = 10000.0;
	m_Kb = 5000.0;
	m_Kd = 0;
	m_Epsilon = 0.01;
	m_Mu = 0.3;
	
	m_bEqualVertexMass = false;
	m_MassDensity = 5.1;
}

CCloth3D::CCloth3D(std::string filepath) : m_pBVHTree(NULL), m_bDeformable(true), m_Gravity(0, 0, -5.0), m_dt(0), m_bShowBV(false)
{
	m_h = 0.01;
	m_Kst = 10000.0;
	m_Ksh = 10000.0;
	m_Kb = 5000.0;
	m_Kd = 0;
	m_Epsilon = 0.01;
	m_Mu = 0.3;
	
	m_bEqualVertexMass = false;
	m_MassDensity = 5.1;

	Create(filepath.c_str());
}


CCloth3D::CCloth3D(const CCloth3D& other)
{
	m_h = other.m_h;
	m_Kst = other.m_Kst;
	m_Ksh = other.m_Ksh;
	m_Kb = other.m_Kb;
	m_Kd = other.m_Kd;
	m_Epsilon = other.m_Epsilon;
	m_Gravity = other.m_Gravity;

	m_VertexArray = other.m_VertexArray;
	m_StrechSpringArray = other.m_StrechSpringArray;
	m_BendSpringArray = other.m_BendSpringArray;
	m_Color = other.m_Color;

	m_bDeformable = other.m_bDeformable;
	m_bEqualVertexMass = other.m_bEqualVertexMass;
	m_bShowBV = other.m_bShowBV;

	m_pBVHTree = new CBVHTree();
	m_pBVHTree->SetDeformable(m_bDeformable);
	m_pBVHTree->BuildBVH(this);
}

CCloth3D::~CCloth3D(void)
{
	Clear();
}

void CCloth3D::Clear()
{
	m_VertexArray.clear();
	m_StrechSpringArray.clear();
	m_BendSpringArray.clear();

	m_Color = COLOR();

	if ( m_pBVHTree )
	{
		delete m_pBVHTree;
		m_pBVHTree = NULL;
	}
}

void CCloth3D::Create(const char* filename)
{
	if ( !Load(filename) )
		return;
	
	Initialize();
}

void CCloth3D::Initialize()
{
	m_bDeformable = true;

	// Build BVH tree
	if ( m_pBVHTree )
		delete m_pBVHTree;

	m_pBVHTree = new CBVHTree();
	m_pBVHTree->SetDeformable(m_bDeformable);
	m_pBVHTree->BuildBVH(this);
	
	unsigned int numVerts = m_VertexArray.size();

	F0.resize(numVerts);
	f0.Create(numVerts);
	V0.resize(numVerts);
	temp.resize(numVerts);
	x.resize(numVerts);
	dF_dX.Create(numVerts, 7);
	dF_dV.Create(numVerts, 7);
	I.Create(numVerts, 1);
	M.Create(numVerts, 1);
	InvM.Create(numVerts, 1);
	A.Create(numVerts, 7);
	Pinv.Create(numVerts, 1);
	
	m_Frame = 0;
	m_Filepath = "c:\\temp\\My Cloth Scenes\\";
	m_Filename = "cloth";
}

bool CCloth3D::Load(const char* filename)
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
			pnt.m_X = (double)atof(sToken.c_str());

			// y
			++iter;
			sToken = (*iter);			
			pnt.m_Y = (double)atof(sToken.c_str());

			// z
			++iter;
			sToken = (*iter);			
			pnt.m_Z = (double)atof(sToken.c_str());

			CVertexCloth3D vert;
			vert.m_Pos.Set(pnt.m_X, pnt.m_Y, pnt.m_Z);
			vert.m_PosOld = vert.m_Pos;

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

			tri.m_Index = m_TriangleArray.size();
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

// This method should be called after area of triangles are calculated. 
void CCloth3D::CalcParticleMassFromDensity()
{
	if ( m_bEqualVertexMass )
		return;

	// Set up m_TriangleIndexes for vertices
	for ( std::vector<CTriangleCloth3D>::iterator iterTri = m_TriangleArray.begin(); iterTri != m_TriangleArray.end(); ++iterTri )
	{
		const CTriangleCloth3D& tri = (*iterTri);

		for ( int i = 0; i < 3; i++ )
		{
			int indexVrx = tri.m_IndexVrx[i];
			m_VertexArray[indexVrx].m_TriangleIndexes.push_back(tri.m_Index);
		}
	}

	double oneThird = 1.0 / 3.0;

	for ( std::vector<CVertexCloth3D>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexCloth3D& vert = *iter;
		double sumA = 0; // sum of triangle areas

		for ( int i = 0; i < (int)vert.m_TriangleIndexes.size(); i++ )
		{
			sumA += m_TriangleArray[vert.m_TriangleIndexes[i]].A;
		}

		vert.m_Mass = oneThird * sumA * m_MassDensity;
		
		// To save memory, clear m_TriangleIndexes for vertices. We wouldn't need it. 
		vert.m_TriangleIndexes.clear();
	}
}

void CCloth3D::FillSpringArray()
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

	// Calculate area, du1, du2, dv1 and dv2 of triangles
	for ( iterTri = m_TriangleArray.begin(); iterTri != m_TriangleArray.end(); ++iterTri )
	{
		CTriangleCloth3D& tri = (*iterTri);
		
		const CVector3D& v0 = m_VertexArray[tri.GetVertexIndex(0)].m_Pos;
		const CVector3D& v1 = m_VertexArray[tri.GetVertexIndex(1)].m_Pos;
		const CVector3D& v2 = m_VertexArray[tri.GetVertexIndex(2)].m_Pos;

		// calculate coordinates of vertices in u, v world. It would not work if the triangle is degenerated. 
		CVector3D uv0(0, 0, 0);
		CVector3D uv1(0, 0, 0);
		CVector3D uv2(0, 0, 0);

		CVector3D v10 = v1 - v0;
		CVector3D u = (v2 - v0).Normalize();
		CVector3D v = (u).Cross(v10).Cross(u).Normalize();

		uv1.m_X = v10.Dot(u);
		uv1.m_Y = v10.Dot(v);
		
		uv2.m_X = (v2 - v0).Dot(u);
		uv2.m_Y = 0;

		tri.du1 = uv1.m_X - uv0.m_X;
		tri.du2 = uv2.m_X - uv0.m_X;
		tri.dv1 = uv1.m_Y - uv0.m_Y;
		tri.dv2 = uv2.m_Y - uv0.m_Y;

		assert((tri.du1*tri.dv2 - tri.du2*tri.dv1) != 0); 

		tri.inv_det = 1.0 / (tri.du1*tri.dv2 - tri.du2*tri.dv1);

		tri.dWu_dX[0] = tri.inv_det*(tri.dv1 - tri.dv2);
		tri.dWu_dX[1] = tri.inv_det*(tri.dv2);
		tri.dWu_dX[2] = tri.inv_det*(-tri.dv1);

		tri.dWv_dX[0] = tri.inv_det*(tri.du2 - tri.du1);
		tri.dWv_dX[1] = tri.inv_det*(-tri.du2);
		tri.dWv_dX[2] = tri.inv_det*(tri.du1);

		tri.A = abs(0.5 * (tri.du1 * tri.dv2 - tri.dv1 * tri.du2));
		//tri.Asqrt = sqrt(tri.A); // This seems to make solver unstable. Don't know why at this moment..
		tri.Asqrt = tri.A;
	}

	CalcParticleMassFromDensity();	
}

void CCloth3D::SetGravity(const CVector3D& gravity)
{
	m_Gravity = gravity;
}

const CVector3D& CCloth3D::GetGravity() const
{
	return m_Gravity;
}

void CCloth3D::SetMassDensity(double massDensity)
{
	m_bEqualVertexMass = false;
	m_MassDensity = massDensity;
	CalcParticleMassFromDensity();
}

void CCloth3D::SetVertexMass(double vertexMass)
{
	m_bEqualVertexMass = true;

	double sumMass = 0;

	for ( std::vector<CVertexCloth3D>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexCloth3D& vert = *iter;
		sumMass += vert.m_Mass;
	}

	double aveMass = sumMass / m_VertexArray.size();

	for ( std::vector<CVertexCloth3D>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexCloth3D& vert = *iter;
		vert.m_Mass = aveMass;
	}
}

void CCloth3D::AddPin(int vertexIndex)
{
	if ( vertexIndex < 0 || vertexIndex >= GetVertexArray().size() )
		return;

	CVertexCloth3D* pVertex = &GetVertexArray()[vertexIndex];
	CClothPin pin(pVertex, pVertex->m_Pos);
	m_PinArray.push_back(pin);

	pVertex->m_pPin = &m_PinArray[m_PinArray.size()-1];
	pVertex->m_PinIndex = m_PinArray.size()-1;
}

void CCloth3D::Render()
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

	/*for ( std::vector<CVector3D>::iterator iter = m_NormalVecArray.begin(); iter != m_NormalVecArray.end(); iter++ )
	{
		(*iter).Normalize();
	}*/

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
	color[0] = 1.0f;
	color[1] = 0.0f;
	color[2] = 0.0f;
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

	// BVH
	if ( m_bShowBV )
		if ( m_pBVHTree )
		{
			m_pBVHTree->Refit();
			m_pBVHTree->Visualize(-1);
		}

	glEnable(GL_LIGHTING);

	static int frame = 0;

	/*if ( frame++ % 5 == 0 )
		Save();*/
}

void CCloth3D::Save()
{
	std::stringstream int2Str;
	int2Str << m_Frame+1;

	std::string filepath = m_Filepath + m_Filename + "." + int2Str.str() + ".obj";

	Save(filepath);
}

void CCloth3D::Save(std::string filePath)
{
	m_outStream.open(filePath.c_str());

	if ( !m_outStream.is_open() )
	{
		m_outStream.close();
		return;
	}

	m_outStream << "# Frame: " << m_Frame << " " << "numVertices: " << m_VertexArray.size() << " " << "numFaces " << m_TriangleArray.size() <<  endl;


	for ( unsigned int i = 0; i < m_VertexArray.size(); i++ )
	{	
		float ax = (float)m_VertexArray[i].m_Pos.m_X;
		float ay = (float)m_VertexArray[i].m_Pos.m_Y;
		float az = (float)m_VertexArray[i].m_Pos.m_Z;

		//m_outStream << "v " << i << " " << ax << " " << ay << " " << az << endl;
		m_outStream << "v " << ax << " " << ay << " " << az << endl;
	}

	for (unsigned int i = 0; i < m_TriangleArray.size(); ++i)
	{
		int idx0 = m_TriangleArray[i].GetVertexIndex(0);
		int idx1 = m_TriangleArray[i].GetVertexIndex(1);
		int idx2 = m_TriangleArray[i].GetVertexIndex(2);

		//m_outStream << "f " << i << " " << idx0 << " " << idx1 << " " << idx2 << endl;
		m_outStream << "f " << idx0+1 << " " << idx1+1 << " " << idx2+1 << endl;
	}

	m_outStream << endl;

	if ( m_outStream.is_open() )
		m_outStream.close();

	m_Frame++;
}

void CCloth3D::CalcForces()
{
	std::vector<CVertexCloth3D>::iterator iter;

	// clear force and impluse
	// f = 0
	for ( iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexCloth3D& vert = *iter;
		vert.m_Force = CVector3D(0, 0, 0);
	}
	
	// gravity
	// f.y -= 9.82 * m
	for ( iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexCloth3D& vert = *iter;
		
		vert.m_Force = vert.m_Mass * m_Gravity;
	}

	//CalcForcesByBaraff();

	// apply stretching springs force and damping
	std::vector<CSpringCloth3D>::const_iterator iterStrechSpring;
	
	for ( iterStrechSpring = m_StrechSpringArray.begin(); iterStrechSpring != m_StrechSpringArray.end(); iterStrechSpring++ )
	{
		const CSpringCloth3D& spring = (*iterStrechSpring);

		CVertexCloth3D& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
		CVertexCloth3D& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

		double len = (vert0.m_Pos - vert1.m_Pos).Length();

		CVector3D vec = (vert1.m_Pos - vert0.m_Pos).Normalize();

		double springForce = m_Kst * (len - spring.GetRestLength());

		vert0.m_Force += vec * springForce;
		vert1.m_Force += -vec * springForce;

		double dampingForce = m_Kd * (vert0.m_Vel - vert1.m_Vel).Dot(vec);

		if ( abs(dampingForce) > abs(springForce) )
			dampingForce = springForce;

		vert0.m_Force += vec * dampingForce;
		vert1.m_Force += -vec * dampingForce;	
	}

	// apply bending spring forces
	std::vector<CSpringCloth3D>::const_iterator iterBendSpring;
	
	for ( iterBendSpring = m_BendSpringArray.begin(); iterBendSpring != m_BendSpringArray.end(); iterBendSpring++ )
	{
		const CSpringCloth3D& spring = (*iterBendSpring);

		CVertexCloth3D& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
		CVertexCloth3D& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

		double len = (vert0.m_Pos - vert1.m_Pos).Length();

		CVector3D vec = (vert1.m_Pos - vert0.m_Pos).Normalize();

		double springForce = m_Kb * (len - spring.GetRestLength());

		vert0.m_Force += vec * springForce;
		vert1.m_Force += -vec * springForce;

		double dampingForce = m_Kd * (vert0.m_Vel - vert1.m_Vel).Dot(vec);

		if ( abs(dampingForce) > abs(springForce) )
			dampingForce = springForce;

		vert0.m_Force += vec * dampingForce;
		vert1.m_Force += -vec * dampingForce;	
	}
}

void CCloth3D::StrainLimiting(double dt)
{
	m_dt = dt;

	std::vector<CVector3D> newPosArray(m_VertexArray.size());

	for ( unsigned int i = 0; i < m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];
		newPosArray[i] = vert.m_Pos + vert.m_Vel * dt;
	}

	unsigned int numIteration = 0;
	unsigned int maxIteration = 10;
	bool bNeedMoreIteration = true;

	while ( bNeedMoreIteration && numIteration++ < maxIteration )
	{
		bNeedMoreIteration = false;
               
		//#pragma omp parallel for
		for ( int i = 0; i < (int)m_StrechSpringArray.size(); i++ )
		{
			//bool bNeedLimiting = false;
			bool bNeedLimiting = true;

			const CSpringCloth3D& spring = m_StrechSpringArray[i];

			CVertexCloth3D& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
			CVertexCloth3D& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

			CVector3D vecNewSpring = newPosArray[vert0.GetIndex()] - newPosArray[vert1.GetIndex()];
			
			double newLen = vecNewSpring.Length();
			double restLen = spring.GetRestLength();
			double desiredLen = 0;
         
			//if ( newLen > 1.1 * restLen ) // allowing maximum 10% stretching
			//{
			//	desiredLen = 1.1 * restLen;
			//	bNeedLimiting = true;
			//}
			//else if ( newLen < 0.999 * restLen ) // we are strict on compression
			//{
			//	desiredLen = 0.999 * restLen;
			//	bNeedLimiting = true;
			//}

			desiredLen = restLen;
			bNeedLimiting = true;
                  
			if ( bNeedLimiting )
			{
				assert(desiredLen > 0.0);

				CVector3D desiredVecNewSpring = vecNewSpring;
				desiredVecNewSpring.Normalize();
				desiredVecNewSpring = desiredLen * desiredVecNewSpring;
            
				CVector3D midPnt = 0.5 * (newPosArray[vert0.GetIndex()] + newPosArray[vert1.GetIndex()]);				
            
				if ( vert0.m_pPin && !vert1.m_pPin ) // if vert0 is pinned and vert1 is not
				{
					newPosArray[vert1.GetIndex()] = newPosArray[vert0.GetIndex()] - desiredVecNewSpring;
				}
				else if ( !vert0.m_pPin && vert1.m_pPin ) // if vert1 is pinned and vert0 is not
				{
					newPosArray[vert0.GetIndex()] = newPosArray[vert1.GetIndex()] + desiredVecNewSpring;
				}
				else if ( !vert0.m_pPin && !vert1.m_pPin ) // if both are not pinned
				{
					newPosArray[vert0.GetIndex()] = midPnt + 0.5 * desiredVecNewSpring;
					newPosArray[vert1.GetIndex()] = midPnt - 0.5 * desiredVecNewSpring;
				}
            
				bNeedMoreIteration = true;
			}         
		}
	}

	for ( unsigned int i = 0; i < m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];
		vert.m_Vel = (newPosArray[i] - vert.m_Pos)/dt;
	}
   
}

void CCloth3D::EnforceEdgeConstraints(double dt)
{
	m_dt = dt;

	std::vector<CVector3D> newPosArray(m_VertexArray.size());

	for ( unsigned int i = 0; i < m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];
		newPosArray[i] = vert.m_Pos + vert.m_Vel * dt;
	}

	int numEdges = m_StrechSpringArray.size();

	// randomize the array
	std::vector<int> ramdomEdgeIndexArray;
	ramdomEdgeIndexArray.reserve(numEdges);

	for ( int i = 0; i < numEdges; i++ )
		ramdomEdgeIndexArray.push_back(i);

	srand(0);

	for (int i = 0; i < (numEdges-1); i++ ) 
	{
		int a = i + (rand() % (numEdges-i)); 
		int temp = ramdomEdgeIndexArray[i]; 
		ramdomEdgeIndexArray[i] = ramdomEdgeIndexArray[a]; 
		ramdomEdgeIndexArray[a] = temp;
	}

	for ( int i = 0; i < (int)m_StrechSpringArray.size(); i++ )
	{
		int indexEdge = ramdomEdgeIndexArray[i];

		bool bNeedLimiting = false;

		const CSpringCloth3D& spring = m_StrechSpringArray[indexEdge];

		CVertexCloth3D& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
		CVertexCloth3D& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

		CVector3D vecNewSpring = newPosArray[vert0.GetIndex()] - newPosArray[vert1.GetIndex()];

		double newLen = vecNewSpring.Length();
		double restLen = spring.GetRestLength();

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

		newPosArray[vert0.GetIndex()] += dVert0;
		newPosArray[vert1.GetIndex()] += dVert1;			  
	}

	for ( unsigned int i = 0; i < m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];
		vert.m_Vel = (newPosArray[i] - vert.m_Pos)/dt;
	}
}

void CCloth3D::EnforceVertexGradientConstraints(double dt)
{
	CVector3D gradient(0, 0, 0);

	int numVerts = m_VertexArray.size();
	
	// randomize the array
	std::vector<int> ramdomVertIndexArray;
	ramdomVertIndexArray.reserve(numVerts);

	for ( int i = 0; i < numVerts; i++ )
		ramdomVertIndexArray.push_back(i);
	
	srand(0);

	for (int i = 0; i < (numVerts-1); i++ ) 
	{
		int a = i + (rand() % (numVerts-i)); 
		int temp = ramdomVertIndexArray[i]; 
		ramdomVertIndexArray[i] = ramdomVertIndexArray[a]; 
		ramdomVertIndexArray[a] = temp;
	}

	//#pragma omp parallel for
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		int index_i = ramdomVertIndexArray[i];
		//int index_i = i;
		CVertexCloth3D& x_i = m_VertexArray[index_i];

		if ( x_i.m_pPin )
			continue;
		
		double Ctot = 0;
		CVector3D gradCtot(0, 0, 0);
		
		gradient = CVector3D(0, 0, 0);
		double a = 0;
				
		for ( int j = 0; j < (int)x_i.m_StrechSpringIndexes.size(); j++ )
		{
			int stretchSpringIndex = j;

			int springIndex = x_i.m_StrechSpringIndexes[stretchSpringIndex];
			int index_j = m_StrechSpringArray[springIndex].GetTheOtherVertexIndex(x_i.GetIndex());
			const CVertexCloth3D& x_j = m_VertexArray[index_j];

			double L0 = m_StrechSpringArray[springIndex].GetRestLength();

			CVector3D vec = (x_j.m_Pos) - (x_i.m_Pos);
			//CVector3D vij = (x_i.m_Pos + x_i.m_Vel*dt) - (x_j.m_Pos + x_j.m_Vel*dt);
			CVector3D vji = (x_j.m_Pos + x_j.m_Vel*dt) - (x_i.m_Pos + x_i.m_Vel*dt);
			//CVector3D vji = x_j.m_Pos - x_i.m_Pos;

			if ( x_i.m_Mass > 0 && L0 > 0 )
			{
				/*double Cij = vji.Length() - L0;
				CVector3D gradCij = vji;

				Ctot += Cij;
				gradCtot +=  gradCij;

				a += gradCij.LengthSqr() / x_j.m_Mass;*/

				double coeff = 0.5; // Newton-Rapson coefficient. 
				double Ks = 1.0; // stiffness [0, 1.0]
				//double Cji = vji.LengthSqr() - L0*L0;
				//double Cji = vji.LengthSqr()/(2.0*L0) - L0/2.0;
				double Cji = (vji.Length() - L0); // Need to divide it by L0?
				//double Cji = (vji.LengthSqr() - L0*L0)/(vji.LengthSqr() + L0*L0);
				
				if ( vji.Length() > 0 )
				{
					//gradient += Ks*coeff*(x_i.m_Mass / (x_i.m_Mass + x_j.m_Mass))*Cji* vji  / (vji.Length());

					gradient += Ks*coeff*(x_i.m_Mass / (x_i.m_Mass + x_j.m_Mass))*Cji* vji  / (vji.Length());

					//gradient += Ks*coeff*(x_i.m_Mass / (x_i.m_Mass + x_j.m_Mass))*(Cji/(vji.LengthSqr()/(L0*L0)) * vji  / (vji.Length()));				

					//gradient += Ks*coeff*(x_i.m_Mass / (x_i.m_Mass + x_j.m_Mass))*(vji.LengthSqr()-L0*L0) * vji  / (vji.LengthSqr());				


					//gradient += Ks*coeff*Cji * vec  / (vec.Length());				
					//gradient += Ks*coeff*((1.0/x_i.m_Mass) / (1.0/x_i.m_Mass + 1.0/x_j.m_Mass))*Cji * vji; // Do I need to normalize vji?			
					//gradient += Ks*coeff*(0.5)*Cji * vji / (vji.Length());				
					//gradient += Ks*coeff*(x_i.m_Mass / (x_i.m_Mass + x_j.m_Mass))*Cji * vec / (vec.Length());				
				}
			}
		}

		/*a = gradCtot.Length();

		double tol = 1e-6;

		if ( a < tol && a > -tol )
		continue;

		CVector3D dPos = -(Ctot / gradCtot.Length()) * gradCtot;

		CVector3D dVel = dPos / dt;*/

		//x_i.m_Vel += dVel;
		
		x_i.m_Vel += gradient/dt;		
	}
}

void CCloth3D::EnforceVertexGradientConstraintsGlobally(double dt)
{
	static CSparseMatrix<double> A;
	static CSparseMatrix<double> Pinv;
	static std::vector<CVector3D> B;
	static std::vector<CVector3D> x;
	bool bFirst = true;

	if ( bFirst )
	{
		A.Create(m_VertexArray.size(), 0);
		Pinv.Create(m_VertexArray.size(), 0);
		B.resize(m_VertexArray.size());
		x.resize(m_VertexArray.size());

		bFirst = false;
	}

	A.ZeroWithoutResize();

	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& x_i = m_VertexArray[i];

		/*if ( x_i.m_pPin )
			continue;*/
		
		CVector3D x_i_posNew = x_i.m_Pos + x_i.m_Vel*dt;
		CVector3D x_i_squared_posNew(x_i_posNew.m_X*x_i_posNew.m_X, x_i_posNew.m_Y*x_i_posNew.m_Y, x_i_posNew.m_Z*x_i_posNew.m_Z);

		B[i] = CVector3D(0, 0, 0);

		for ( int j = 0; j < (int)x_i.m_StrechSpringIndexes.size(); j++ )
		{
			A.AddToElement(i, i, 1.0);
			
			int springIndex = x_i.m_StrechSpringIndexes[j];
			int index_j = m_StrechSpringArray[springIndex].GetTheOtherVertexIndex(x_i.GetIndex());
			const CVertexCloth3D& x_j = m_VertexArray[index_j];

			A.SetElement(i, index_j, -1.0);

			double L0 = m_StrechSpringArray[springIndex].GetRestLength();

			CVector3D x_j_posNew = x_j.m_Pos + x_j.m_Vel*dt;
			CVector3D x_j_squared_posNew(x_j_posNew.m_X*x_j_posNew.m_X, x_j_posNew.m_Y*x_j_posNew.m_Y, x_j_posNew.m_Z*x_j_posNew.m_Z);

			B[i] += -(L0)*(x_j_posNew - x_i_posNew).Normalize();
			//B[i] += -(L0)*(x_j_squared_posNew - x_i_squared_posNew).Normalize();
		}

		x[i] = x_i_posNew;
		//x[i] = x_i_squared_posNew;
	}

	int maxIteraion = 200;
	/*CMINRESSolver<double, CVector3D> MINRESSolver;
	int numIterations = MINRESSolver.Solve(A, B, x, NULL, 1e-6, maxIteraion);*/

	for ( unsigned int i = 0; i < A.Size(); i++ )
	{
		const double a = A.GetElement(i, i);
		if ( a != 0 )
			Pinv.SetElement(i, i, 1.0/a);
		else
			Pinv.SetElement(i, i, 0);
	}

	CCGSolver<double, CVector3D> CGSolver;
	int numIterations = CGSolver.Solve(A, B, x, Pinv, 1e-7, maxIteraion);

	/*if ( numIterations == 0 || numIterations >= maxIteraion )
		return;*/
	
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& x_i = m_VertexArray[i];
		x_i.m_Vel = (x[i] - x_i.m_Pos) / dt;
		//x_i.m_Vel = (CVector3D(std::sqrt(x[i].m_X), std::sqrt(x[i].m_Y), std::sqrt(x[i].m_Z)) - x_i.m_Pos) / dt;
	}
}

void CCloth3D::EnforceVertexGradientConstraintsGlobally2(double dt)
{
	CSparseMatrix<double> AA;
	CSparseMatrix<double> A;
	CSparseMatrix<double> trA;
	std::vector<CVector3D> B;
	std::vector<CVector3D> BB;
	std::vector<CVector3D> x;
	std::vector<CVector3D> p;

	AA.Create(m_VertexArray.size(), 0);
	A.Create(m_VertexArray.size(), 0);
	trA.Create(m_VertexArray.size(), 0);
	B.resize(m_VertexArray.size());
	BB.resize(m_VertexArray.size());
	x.resize(m_VertexArray.size());
	p.resize(m_VertexArray.size());

	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& x_i = m_VertexArray[i];

		/*if ( x_i.m_pPin )
			continue;*/
		
		CVector3D x_i_posNew = x_i.m_Pos + x_i.m_Vel*dt;

		B[i] = CVector3D(0, 0, 0);
		x[i] = x_i_posNew;

		for ( int j = 0; j < (int)x_i.m_StrechSpringIndexes.size(); j++ )
		{
			A.AddToElement(i, i, 1.0);
			
			int springIndex = x_i.m_StrechSpringIndexes[j];
			int index_j = m_StrechSpringArray[springIndex].GetTheOtherVertexIndex(x_i.GetIndex());
			const CVertexCloth3D& x_j = m_VertexArray[index_j];

			A.SetElement(i, index_j, -1.0);

			double L0 = m_StrechSpringArray[springIndex].GetRestLength();

			CVector3D x_j_posNew = x_j.m_Pos + x_j.m_Vel*dt;

			B[i] += -(L0)*(x_j_posNew - x_i_posNew).Normalize();
		}
	}

	A.GetTranspose(&trA);
	//AA = trA.Mult(A);
	Multiply(trA, A, AA);
	Multiply(trA, B, BB);

	int maxIteraion = 200;
	CMINRESSolver<double, CVector3D> MINRESSolver;
	//int numIterations = MINRESSolver.Solve(A, B, x, NULL, false, 1e-6, maxIteraion);
	
	//for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	//{
	//	CVertexCloth3D& x_i = m_VertexArray[i];

	//	/*if ( x_i.m_pPin )
	//		continue;*/
	//	
	//	CVector3D x_i_posNew = x_i.m_Pos;

	//	B[i] = CVector3D(0, 0, 0);
	//	p[i] = x_i_posNew;

	//	for ( int j = 0; j < (int)x_i.m_StrechSpringIndexes.size(); j++ )
	//	{
	//		int springIndex = x_i.m_StrechSpringIndexes[j];
	//		int index_j = m_StrechSpringArray[springIndex].GetTheOtherVertexIndex(x_i.GetIndex());
	//		const CVertexCloth3D& x_j = m_VertexArray[index_j];

	//		double L0 = m_StrechSpringArray[springIndex].GetRestLength();

	//		CVector3D x_j_posNew = x_j.m_Pos;

	//		B[i] += -(L0)*(x_j_posNew - x_i_posNew).Normalize();
	//	}
	//}

	//numIterations = MINRESSolver.Solve(A, B, p, NULL, false, 1e-6, maxIteraion);
	int numIterations = MINRESSolver.Solve(AA, BB, p, NULL, false, 1e-6, maxIteraion);

	/*CCGSolver<double, CVector3D> CGSolver;
	int numIterations = CGSolver.Solve(A, B, x, false, 1e-6, maxIteraion);*/

	/*if ( numIterations == 0 || numIterations >= maxIteraion )
		return;*/
	
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& x_i = m_VertexArray[i];
		x_i.m_Vel += (x[i] - p[i]) / dt;
		//x_i.m_Vel = (x[i] - x_i.m_Pos) / dt;
	}
}

void CCloth3D::EnforceEdgeConstraintsGlobally(double dt)
{
	static CSparseMatrix<double> AA;
	static CSparseMatrix<double> A;
	static CSparseMatrix<double> trA;
	static std::vector<CVector3D> b;
	static std::vector<CVector3D> bb;
	static std::vector<CVector3D> x;
	static bool bFirst = true;

	int n = m_StrechSpringArray.size();
	int m = m_VertexArray.size();

	if ( bFirst )
	{
		AA.Create(m, m, 0);
		A.Create(n, m, 0);
		trA.Create(m, n, 0);
		b.resize(n);
		bb.resize(m);
		x.resize(m);

		bFirst = false;
	}

	AA.ZeroWithoutResize();
	A.ZeroWithoutResize();
	trA.ZeroWithoutResize();

	for ( int i = 0; i < n; i++ )
	{
		CSpringCloth3D& e_i = m_StrechSpringArray[i];

		CVertexCloth3D& p0 = m_VertexArray[e_i.GetVertexIndex(0)];
		CVertexCloth3D& p1 = m_VertexArray[e_i.GetVertexIndex(1)];

		A.SetElement(i, p0.GetIndex(), 1);
		A.SetElement(i, p1.GetIndex(), -1);

		/*CVector3D newPos0 = p0.m_Pos + p0.m_Vel * dt;
		CVector3D newPos1 = p1.m_Pos + p1.m_Vel * dt;*/

		CVector3D newPos0 = p0.m_Pos + p0.m_Vel * dt;
		CVector3D newPos1 = p1.m_Pos + p1.m_Vel * dt;

		b[i] = (newPos0 - newPos1).Normalize() * e_i.GetRestLength();
		//x[i] = newPos0;
	}

	A.GetTranspose(&trA);
	//AA = trA.Mult(A);
	Multiply(trA, A, AA);
	Multiply(trA, b, bb);

	int maxIteraion = 200;
	/*CMINRESSolver<double, CVector3D> MINRESSolver;
	int numIterations = MINRESSolver.Solve(AA, bb, x, NULL, false, 1e-10, maxIteraion);*/

	CCGSolver<double, CVector3D> CGSolver;
	int numIterations = CGSolver.Solve(AA, bb, x, false, 1e-6, maxIteraion);

	if ( numIterations == 0 || numIterations >= maxIteraion )
		return;
	
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& x_i = m_VertexArray[i];
		//x_i.m_Vel += (x[i] - p[i]) / dt;
		x_i.m_Vel = (x[i] - x_i.m_Pos) / dt;
	}
}

double CCloth3D::CalcConstraint(int indexEdge, int indexVertex, double dt, CVector3D* pGradientOfConstraint/* = NULL*/)
{
	assert( 0 <= indexEdge && indexEdge < m_StrechSpringArray.size());
	assert( 0 <= indexVertex && indexVertex < m_VertexArray.size() );

	CSpringCloth3D& e = m_StrechSpringArray[indexEdge];

	CVertexCloth3D& vert = m_VertexArray[indexVertex];
	CVertexCloth3D& p0 = m_VertexArray[e.GetVertexIndex(0)];
	CVertexCloth3D& p1 = m_VertexArray[e.GetVertexIndex(1)];

	CVector3D newP0;
	CVector3D newP1;

	if ( !p0.m_pPin )
		newP0 = p0.m_Pos + p0.m_Vel * dt;
	else
		newP0 = m_PinArray[p0.m_PinIndex].GetPinPos() + m_PinArray[p0.m_PinIndex].GetPinVelocity() * dt;

	if ( !p1.m_pPin )
		newP1 = p1.m_Pos + p1.m_Vel * dt;
	else
		newP1 = m_PinArray[p1.m_PinIndex].GetPinPos() + m_PinArray[p1.m_PinIndex].GetPinVelocity() * dt;

	double L0 = e.GetRestLength();
	CVector3D vec = newP1 - newP0;
	double constraint = 0;

	double l = vec.Length();

	if ( L0 > 0 )
		//constraint = (vec.LengthSqr() / L0) - L0;
		constraint = ((vec.LengthSqr() / L0) - L0)/2.0;
		//constraint = ((l*l*l / (L0*L0)) - L0)/3.0;
		//constraint = (vec.Length()) - L0;
		//constraint = (vec.LengthSqr()) - L0*L0;

	if ( pGradientOfConstraint != NULL )
	{
		if ( vert.GetIndex() == p0.GetIndex() )
		{
			if ( L0 > 0 && !p0.m_pPin )
				//*pGradientOfConstraint = -(2.0/L0)*vec;
				*pGradientOfConstraint = -vec/L0;
				//*pGradientOfConstraint = -l*vec/(L0*L0);
				//*pGradientOfConstraint = -vec.Normalize();
				//*pGradientOfConstraint = -(2.0)*vec;		
			else
				*pGradientOfConstraint = CVector3D(0, 0, 0);

			/*if ( p1.m_pPin )
				*pGradientOfConstraint *= 2.0;*/
		}
		else if ( vert.GetIndex() == p1.GetIndex() )
		{
			if ( L0 > 0 && !p1.m_pPin )
				//*pGradientOfConstraint = (2.0/L0)*vec;
				*pGradientOfConstraint = vec/L0;
				//*pGradientOfConstraint = l*vec/(L0*L0);
				//*pGradientOfConstraint = vec.Normalize();
				//*pGradientOfConstraint = (2.0)*vec;	
			else
				*pGradientOfConstraint = CVector3D(0, 0, 0);

			/*if ( p0.m_pPin )
				*pGradientOfConstraint *= 2.0;*/
		}
		else
		{
			constraint = 0;
			*pGradientOfConstraint = CVector3D(0, 0, 0);
		}
	}

	if ( p0.m_pPin && p1.m_pPin )
		constraint = 0;

	return constraint;
}

void CCloth3D::EnforceFaseProjectionGlobally(double dt)
{
	// n:# of vertives, m:# of edges
	static CSparseMatrix<double> A; // gradC * InvM * tranpose(gradC). size:m x m
	static CSparseMatrix<CVector3D> gradCTr; // Transpose of gradient of constraints. size: m x n
	static std::vector<double> C; // Contraints. size:m
	static std::vector<double> dLamda; // delta lamda(Lagrange Multiplier). size:m. 
										  // This is unknown and what we are solving for.
	static std::vector<CVector3D> dX;

	static bool bFirst = true;
	double dt2 = dt*dt;

	int n = m_VertexArray.size();
	int m = m_StrechSpringArray.size();

	if ( bFirst )
	{
		A.Create(m, m, 0);
		gradCTr.Create(n, m, 0);
		C.resize(m);
		dLamda.resize(m);
		dX.resize(n);

		bFirst = false;
	}

	A.ZeroWithoutResize();
	InvM.ZeroWithoutResize();
	gradCTr.ZeroWithoutResize();
	
	for ( int i = 0; i < m; i++ )
	{
		CSpringCloth3D& e = m_StrechSpringArray[i];
		int edgeIndex = e.GetIndex();

		CVertexCloth3D& p0 = m_VertexArray[e.GetVertexIndex(0)];
		CVertexCloth3D& p1 = m_VertexArray[e.GetVertexIndex(1)];
					
		CVector3D gradC0, gradC1;
		double constraint = CalcConstraint(edgeIndex, p0.GetIndex(), dt, &gradC0);
		CalcConstraint(edgeIndex, p1.GetIndex(), dt, &gradC1);

		C[edgeIndex] = constraint;

		for ( int j = 0; j < (int)p0.m_StrechSpringIndexes.size(); j++ )
		{
			CVector3D gradCother(0, 0, 0);
			int otherEdgeIndex = p0.m_StrechSpringIndexes[j];

			/*if ( edgeIndex == otherEdgeIndex )
				continue;*/

			CalcConstraint(otherEdgeIndex, p0.GetIndex(), dt, &gradCother);
			A.AddToElement(edgeIndex, otherEdgeIndex, dt2*(1.0/p0.m_Mass)*gradC0.Dot(gradCother));
		}

		for ( int j = 0; j < (int)p1.m_StrechSpringIndexes.size(); j++ )
		{
			CVector3D gradCother(0, 0, 0);
			int otherEdgeIndex = p1.m_StrechSpringIndexes[j];
			
			/*if ( edgeIndex == otherEdgeIndex )
				continue;*/

			CalcConstraint(otherEdgeIndex, p1.GetIndex(), dt, &gradCother);
			A.AddToElement(edgeIndex, otherEdgeIndex, dt2*(1.0/p1.m_Mass)*gradC1.Dot(gradCother));
		}

		A.AddToElement(edgeIndex, edgeIndex, dt2*((1.0/p0.m_Mass)*gradC0.Dot(gradC0) + (1.0/p1.m_Mass)*(gradC1).Dot(gradC1)));
		
		gradCTr.SetElement(p0.GetIndex(), edgeIndex, gradC0);
		gradCTr.SetElement(p1.GetIndex(), edgeIndex, gradC1);
	}
	
	int maxIteraion = 200;
	/*CMINRESSolver<double, double> MINRESSolver;
	int numIterations = MINRESSolver.Solve(A, C, dLamda, NULL, false, 1e-10, maxIteraion);*/

	CCGSolver4<double, double> CGSolver;
	int numIterations = CGSolver.Solve(A, C, dLamda, false, 1e-6, maxIteraion);

	if ( numIterations == 0 || numIterations >= maxIteraion )
		return;
	
	Multiply(gradCTr, dLamda, dX);

	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& x_i = m_VertexArray[i];

		if ( x_i.m_pPin )
			continue;

		// Below two lines are the same as the line after them. It is just compact.
		//CVector3D newPos = x_i.m_Pos + x_i.m_Vel * dt - dt2 * (1.0/x_i.m_Mass) * dX[i];
		//x_i.m_Vel = (newPos - x_i.m_Pos) / dt;
		x_i.m_Vel -= dt * (1.0/x_i.m_Mass) * dX[i];
	}
}

void CCloth3D::AdvancePosition(double dt)
{
	m_dt = dt;
	// integrate position
	// p += v * dt

	/*for ( std::vector<CVertexCloth3D>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexCloth3D& vert = *iter;				
		vert.m_Pos = vert.m_Pos + vert.m_Vel * dt;
	}*/

	#pragma omp parallel for
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];	
		//vert.m_Vel.m_Y = 0;
		vert.m_Pos = vert.m_Pos + vert.m_Vel * dt;
	}

	#pragma omp parallel for
	for ( int i = 0; i < (int)m_PinArray.size(); i++ )
	{
		CClothPin& pin = m_PinArray[i];
		CVertexCloth3D* vert = pin.GetPinnedVertex();		
		vert->m_PosOld = vert->m_Pos;
		vert->m_Pos = pin.GetPinPos() + pin.GetPinVelocity() * dt;
		vert->m_Vel = (vert->m_Pos - vert->m_PosOld) / m_dt;
	}
	
	//if ( m_pBVHTree )
	//	m_pBVHTree->Refit();
}

void CCloth3D::IntegrateEuler(double dt)
{
	m_dt = dt;
	CalcForces();

	// integrate velocity
	// v += (f/m) * dt
	//#pragma omp parallel for
	//for ( std::vector<CVertexCloth3D>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];
		
		if ( vert.m_Mass > 0 )			
			vert.m_Vel += (vert.m_Force / vert.m_Mass) * dt;
	}
}

void CCloth3D::IntegrateOnlyGravity(double dt)
{
	m_dt = dt;

	//#pragma omp parallel for
	//for ( std::vector<CVertexCloth3D>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];
		
		if ( vert.m_Mass > 0 )			
			vert.m_Vel += (m_Gravity / vert.m_Mass) * dt;
	}
}


void CCloth3D::IntegrateVerlet(double dt)
{
	m_dt = dt;
	CalcForces();

	// verlet integration
	// Xn+1 = Xn + (Xn - Xn-1) + a * dt^2 
	// a = f/m
	//#pragma omp parallel for
	//for ( std::vector<CVertexCloth3D>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];
		
		if ( vert.m_Mass > 0 )
		{
			CVector3D temp = vert.m_Pos;
			CVector3D newPos = 2.0*vert.m_Pos - vert.m_PosOld + (vert.m_Force/vert.m_Mass)*dt*dt;
			vert.m_PosOld = temp;
			vert.m_Vel = (newPos - vert.m_PosOld) / dt;
		}
	}
}

void CCloth3D::IntegrateRK4(double dt)
{
	m_dt = dt;
	std::vector<CVector3D> dP(m_VertexArray.size());
	std::vector<CVector3D> dV(m_VertexArray.size());

	std::vector<CVector3D> P(m_VertexArray.size());
	std::vector<CVector3D> V(m_VertexArray.size());

	// k1
	CalcForces();

	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];
		
		P[i] = vert.m_Pos;
		V[i] = vert.m_Vel;

		if ( vert.m_Mass > 0 )		
		{
			dV[i] = (vert.m_Force / vert.m_Mass) * dt;
			dP[i] = dV[i] * dt;

			//P[i] += (1.0/6.0)*dP[i];
			V[i] += (1.0/6.0)*dV[i];

			vert.m_Vel += 0.5 * dV[i];
			vert.m_Pos += 0.5 * dP[i];
		}
	}

	// k2
	CalcForces();

	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];
				
		if ( vert.m_Mass > 0 )		
		{
			dV[i] = (vert.m_Force / vert.m_Mass) * dt;
			dP[i] = dV[i] * dt;

			//P[i] += (1.0/3.0)*dP[i];
			V[i] += (1.0/3.0)*dV[i];

			vert.m_Vel += 0.5 * dV[i];
			vert.m_Pos += 0.5 * dP[i];
		}
	}

	// k3
	CalcForces();

	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];
				
		if ( vert.m_Mass > 0 )		
		{
			dV[i] = (vert.m_Force / vert.m_Mass) * dt;
			dP[i] = dV[i] * dt;

			//P[i] += (1.0/3.0)*dP[i];
			V[i] += (1.0/3.0)*dV[i];

			vert.m_Vel += dV[i];
			vert.m_Pos += dP[i];
		}
	}

	// k4
	CalcForces();

	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];
				
		if ( vert.m_Mass > 0 )		
		{
			dV[i] = (vert.m_Force / vert.m_Mass) * dt;
			//dP[i] = dV[i] * dt;

			//P[i] += (1.0/6.0)*dP[i];
			V[i] += (1.0/6.0)*dV[i];

			vert.m_Vel = V[i];
			vert.m_Pos = P[i];
		}
	}
}

void CCloth3D::IntegrateByPositionContraints(double dt)
{
	m_dt = dt;

	std::vector<CVertexCloth3D>::iterator iter;

	// clear force and impulse
	// f = 0
	#pragma omp parallel for
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];
		vert.m_Force = CVector3D(0, 0, 0);
	}

	// gravity
	// f.y -= 9.82 * m
	#pragma omp parallel for
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];		
		vert.m_Force = vert.m_Mass * m_Gravity;
	}

	// apply bending spring forces
	std::vector<CSpringCloth3D>::const_iterator iterBendSpring;

	for ( iterBendSpring = m_BendSpringArray.begin(); iterBendSpring != m_BendSpringArray.end(); iterBendSpring++ )
	{
		const CSpringCloth3D& spring = (*iterBendSpring);

		CVertexCloth3D& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
		CVertexCloth3D& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

		double len = (vert0.m_Pos - vert1.m_Pos).Length();

		CVector3D vec = (vert1.m_Pos - vert0.m_Pos).Normalize();

		double springForce = m_Kb * (len - spring.GetRestLength());

		/*vert0.m_Force += vec * springForce;
		vert1.m_Force += -vec * springForce;

		double dampingForce = m_Kd * (vert0.m_Vel - vert1.m_Vel).Dot(vec);

		if ( abs(dampingForce) > abs(springForce) )
			dampingForce = springForce;

		vert0.m_Force += vec * dampingForce;
		vert1.m_Force += -vec * dampingForce;	*/
	}

	#pragma omp parallel for
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& vert = m_VertexArray[i];

		if ( vert.m_Mass > 0 && !vert.m_pPin )			
			vert.m_Vel += (vert.m_Force / vert.m_Mass) * dt;
	}

	//// Assign random velocity for testing..
	//srand(0);

	//for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	//{
	//	CVertexCloth3D& vert = m_VertexArray[i];

	//	if ( vert.m_Mass > 0 && !vert.m_pPin )			
	//		vert.m_Vel = CVector3D((double)(rand() % 100 - 50) * 0.01, (double)(rand() % 100 - 50) * 0.01, (double)(rand() % 100 - 50) * 0.01);
	//	
	//}

	const double strainLimit = 0.1;
	unsigned int numIteration = 0;
	const unsigned int maxIteration = 5;
	bool bNeedMoreIteration = true;

	double strainLimitMaxSqr = (1.0 + strainLimit) * (1.0 + strainLimit);
	double strainLimitMinSqr = (1.0 - strainLimit) * (1.0 - strainLimit);

	while ( bNeedMoreIteration && numIteration < maxIteration )
	{
		bNeedMoreIteration = false;

		EnforceEdgeConstraints(dt);
		//EnforceVertexGradientConstraints(dt);
		//EnforceVertexGradientConstraintsGlobally(dt);
		//EnforceVertexGradientConstraintsGlobally2(dt);
		//EnforceEdgeConstraintsGlobally(dt);
		//EnforceFaseProjectionGlobally(dt);
		
		for ( int i = 0; i < (int)m_StrechSpringArray.size(); i++ )
		{
			const CSpringCloth3D& spring = m_StrechSpringArray[i];

			CVertexCloth3D& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
			CVertexCloth3D& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

			CVector3D vecNewSpring = vert0.m_Pos - vert1.m_Pos;

			double newLenSqr = vecNewSpring.LengthSqr();
			double restLenSqr = spring.GetRestLength()*spring.GetRestLength();

			if ( newLenSqr > strainLimitMaxSqr * restLenSqr || newLenSqr < strainLimitMinSqr * restLenSqr ) 
			{
				bNeedMoreIteration = true;
				break;
			}
		}

		++numIteration;
	}

	m_NumIter = numIteration;
}


template<class T>
void CCloth3D::Filter(std::vector<T>& a, const void* pCloth /*=NULL*/)
{
	if ( !pCloth )
		return;

	const CCloth3D* pClothObj = (CCloth3D*)pCloth;

	#pragma omp parallel for
	for ( int i = 0; i < (int)a.size(); i++ )
	{
		const CVertexCloth3D& vert = pClothObj->GetVertexArray()[i];	
		
		if ( vert.m_pPin )
			a[i] = 0;
	}
}

bool CCloth3D::ImplicitEulerIntegration(double dt)
{
	m_dt = dt;
	double h = dt;

	// Solve: (I - h*InvM*dF_dV - h*h*InvM*dF_dX) * dV = h*InvM*(F0 + h*dF_dX*V0)
	// where InvM is a inverse of M
	// M is a mass matrix. It is diagonal block matrix whose element is 3 x 3 matrix. 
	// I is an identity block matrix whose element is 3 x 3 identity matrix. 
	// dV is unknown and what we are solving for. 
	// h is time step. h = dt
	
	A.ZeroWithoutResize();
	I.ZeroWithoutResize();
	M.ZeroWithoutResize();
	//InvM.ZeroWithoutResize();
	dF_dV.ZeroWithoutResize();
	dF_dX.ZeroWithoutResize();
	Pinv.ZeroWithoutResize();

	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& v_i = m_VertexArray[i];
		
		double h_over_mass	= h / v_i.m_Mass;
		double h_h_over_mass = h * h_over_mass;	

		// Below is experimental. Need to figure out how to make implicit solver more stable.
		if ( h_over_mass > 1.0 )
		{
			h_over_mass	= h;
			h_h_over_mass = h * h_over_mass;
			I.SetElement(v_i.GetIndex(), v_i.GetIndex(), CMatrix33::IDENTITY*v_i.m_Mass);
		}
		else
			I.SetElement(v_i.GetIndex(), v_i.GetIndex(), CMatrix33::IDENTITY);

		F0[v_i.GetIndex()] = m_Gravity * v_i.m_Mass * h_over_mass;

		f0[v_i.GetIndex()] = m_Gravity * v_i.m_Mass;

		V0[v_i.GetIndex()] = v_i.m_Vel;
		dF_dX.SetElement(v_i.GetIndex(), v_i.GetIndex(), CMatrix33::ZERO);
		dF_dV.SetElement(v_i.GetIndex(), v_i.GetIndex(), CMatrix33::ZERO);
		
		//M.SetElement(v_i.GetIndex(), v_i.GetIndex(), CMatrix33::IDENTITY*v_i.m_Mass);
		//InvM.SetElement(v_i.GetIndex(), v_i.GetIndex(), CMatrix33::IDENTITY/v_i.m_Mass);

		// stretch forces
		for ( std::vector<int>::const_iterator iter = v_i.m_StrechSpringIndexes.begin(); iter != v_i.m_StrechSpringIndexes.end(); iter++ )
		{
			int index_j = m_StrechSpringArray[(*iter)].GetTheOtherVertexIndex(v_i.GetIndex());
			const CVertexCloth3D& v_j = m_VertexArray[index_j];

			double L0 = m_StrechSpringArray[(*iter)].GetRestLength();

			const CVector3D Xij = v_j.m_Pos - v_i.m_Pos;

			if ( Xij.Length() - L0 <= 0 )
				continue;

			CVector3D XijN = Xij;
			XijN.Normalize();
			const CVector3D Vij = v_j.m_Vel - v_i.m_Vel;

			//-------------
			// F0
			//-------------
			const CMatrix33 out_X = Xij.Out(Xij);
			const double inn_X = Xij.Dot(Xij);
			const CMatrix33 outOverInn = out_X / inn_X;

			// stretch spring force
			CVector3D fi = m_Kst * ( Xij.Length() - L0 ) * XijN;
			//CVector3D di = m_Kd * Vij;
			CVector3D di = m_Kd * (Vij.Dot(Xij))*XijN;			
			CVector3D fsum = fi + di;

			F0[v_i.GetIndex()] += fsum * h_over_mass;
			f0[v_i.GetIndex()] += fsum;

			//-------------
			// df_dX
			//-------------
			CMatrix33 df_dx = m_Kst * (CMatrix33::IDENTITY - (L0/Xij.Length())*(CMatrix33::IDENTITY-outOverInn));

			df_dx = df_dx * h_h_over_mass * 3.0/2.0;
	
			dF_dX.SetElement(v_i.GetIndex(), v_j.GetIndex(), df_dx);
			dF_dX.AddToElement(v_i.GetIndex(), v_i.GetIndex(), -df_dx);

			//-------------
			// df_dV
			//-------------
			//CMatrix33 df_dv = m_Kd * CMatrix33::IDENTITY;
			CMatrix33 df_dv = m_Kd * outOverInn;
			df_dv = df_dv * h_over_mass;

			dF_dV.SetElement(v_i.GetIndex(), v_j.GetIndex(), df_dv);
			dF_dV.AddToElement(v_i.GetIndex(), v_i.GetIndex(), -df_dv);
		}

		// bending forces
		for ( std::vector<int>::const_iterator iter = v_i.m_BendSpringIndexes.begin(); iter != v_i.m_BendSpringIndexes.end(); iter++ )
		{
			int index_j = m_BendSpringArray[(*iter)].GetTheOtherVertexIndex(v_i.GetIndex());
			const CVertexCloth3D& v_j = m_VertexArray[index_j];

			double L0 = m_BendSpringArray[(*iter)].GetRestLength();

			const CVector3D Xij = v_j.m_Pos - v_i.m_Pos;

			if ( Xij.Length() - L0 >= 0 )
				continue;

			CVector3D XijN = Xij;
			XijN.Normalize();
			const CVector3D Vij = v_j.m_Vel - v_i.m_Vel;

			//-------------
			// F0
			//-------------
			const CMatrix33 out_X = Xij.Out(Xij);
			const double inn_X = Xij.Dot(Xij);
			const CMatrix33 outOverInn = out_X / inn_X;

			// bending spring force
			CVector3D fi = m_Kb * ( Xij.Length() - L0 ) * XijN;

			//double s = Xij.Length() / L0;
			//CVector3D fi = (-11.541*s*s*s*s + 34.193*s*s*s -39.083*s*s + 23.116*s - 9.713) * m_Kb * XijN;

			//CVector3D di = m_Kd * Vij;
			CVector3D di = m_Kd * (Vij.Dot(Xij))*XijN;
			CVector3D fsum = fi + di;
			F0[v_i.GetIndex()] += fsum * h_over_mass;
			f0[v_i.GetIndex()] += fsum;

			//-------------
			// df_dX
			//-------------
			CMatrix33 df_dx = m_Kb * (CMatrix33::IDENTITY - (L0/Xij.Length())*(CMatrix33::IDENTITY-outOverInn));
			//CMatrix33 df_dx = (1.0/L0) * (-4*11.541*s*s*s + 3*34.193*s*s - 2*39.083*s + 23.116) * outOverInn;

			df_dx = df_dx * h_h_over_mass * 3.0/2.0;
	
			dF_dX.SetElement(v_i.GetIndex(), v_j.GetIndex(), df_dx);
			dF_dX.AddToElement(v_i.GetIndex(), v_i.GetIndex(), -df_dx);

			//-------------
			// df_dV
			//-------------
			//CMatrix33 df_dv = m_Kd * CMatrix33::IDENTITY;
			CMatrix33 df_dv = m_Kd * outOverInn;
			
			df_dv = df_dv * h_over_mass;

			dF_dV.SetElement(v_i.GetIndex(), v_j.GetIndex(), df_dv);
			dF_dV.AddToElement(v_i.GetIndex(), v_i.GetIndex(), -df_dv);
		}
	}

	// Solve: (I - h*invM*dF_dV - h*h*invM*dF_dX) * dV = h*invM*(F0 + h*dF_dX*V0)
	
	bool bImplicit = true;

	if ( bImplicit )
	{
		A = I - dF_dV - dF_dX;
		Multiply(dF_dX, V0, temp);
		Add(F0, temp, b);
	}
	else
	{
		A = I;
		b = F0;
	}

	for ( unsigned int i = 0; i < A.Size(); i++ )
	{
		const CMatrix33& a = A.GetElement(i, i);
		Pinv.SetElement(i, i, a.InverseOther());
	}
		
	int maxIteration = 200;
	m_NumIter = cgSolver.SolveFiltered(A, b, x, Pinv, &Filter<CVector3D>, this, 1e-6, maxIteration);
	//m_NumIter = cgSolver.Solve(A, b, x, Pinv);

	// If solver fails to converge
	if ( m_NumIter == maxIteration )
		return false;

	//x = F0;

	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{		
		CVertexCloth3D& v_i = m_VertexArray[i];

		CVector3D& dv = x[v_i.GetIndex()];
		//v_i.m_Vel += dv;

		//CVector3D dX = dt * (v_i.m_Vel + 3.0/2.0 * dv);
		v_i.m_Vel = v_i.m_Vel + 3.0/2.0 * dv;		
	}

	return true;
}



void CCloth3D::IntegrateBackwardEulerByBW(double dt)
{
	m_dt = dt;
	double h = dt;
		
	A.ZeroWithoutResize();
	I.ZeroWithoutResize();
	M.ZeroWithoutResize();
	InvM.ZeroWithoutResize();
	dF_dV.ZeroWithoutResize();
	dF_dX.ZeroWithoutResize();

	// initialize values
	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& v_i = m_VertexArray[i];
		
		//double h_over_mass	= h / v_i.m_Mass;
		//double h_h_over_mass = h * h_over_mass;	
		
		F0[v_i.GetIndex()] = m_Gravity * v_i.m_Mass * h / v_i.m_Mass;
		//F0[v_i.GetIndex()] = CVector3D(0, 0, 0);
		V0[v_i.GetIndex()] = v_i.m_Vel;
		dF_dX.SetElement(v_i.GetIndex(), v_i.GetIndex(), CMatrix33::ZERO);
		dF_dV.SetElement(v_i.GetIndex(), v_i.GetIndex(), CMatrix33::ZERO);
		I.SetElement(v_i.GetIndex(), v_i.GetIndex(), CMatrix33::IDENTITY);
		M.SetElement(v_i.GetIndex(), v_i.GetIndex(), CMatrix33::IDENTITY*v_i.m_Mass/h);
		//InvM.SetElement(v_i.GetIndex(), v_i.GetIndex(), CMatrix33::IDENTITY/v_i.m_Mass);
	}

	for ( std::vector<CTriangleCloth3D>::iterator iterTri = m_TriangleArray.begin(); iterTri != m_TriangleArray.end(); ++iterTri )
	{
		CTriangleCloth3D& tri = (*iterTri);
		
		const CVector3D& p0 = m_VertexArray[tri.GetVertexIndex(0)].m_Pos;
		const CVector3D& p1 = m_VertexArray[tri.GetVertexIndex(1)].m_Pos;
		const CVector3D& p2 = m_VertexArray[tri.GetVertexIndex(2)].m_Pos;

		CVector3D dX1 = p1 - p0;
		CVector3D dX2 = p2 - p0;

		tri.Wu = tri.inv_det * (tri.dv2 * dX1 - tri.dv1 * dX2);
		tri.Wv = tri.inv_det * (-tri.du2 * dX1 + tri.du1 * dX2);

		double WuLen = tri.Wu.Length();
		double WvLen = tri.Wv.Length();
		double inv_WuLen = 1.0 / WuLen;
		double inv_WvLen = 1.0 / WvLen;

		double Cu = tri.Asqrt*(WuLen - 1.0);
		double Cv = tri.Asqrt*(WvLen - 1.0);

		CVector3D dCu_dX[3];
		CVector3D dCv_dX[3];

		// stretch force
		for ( int i = 0; i < 3; i++ )
		{
			dCu_dX[i] = tri.Asqrt * tri.dWu_dX[i] * tri.Wu * inv_WuLen;
			dCv_dX[i] = tri.Asqrt * tri.dWv_dX[i] * tri.Wv * inv_WvLen;

			CVector3D f = -m_Kst * (dCu_dX[i] * Cu + dCv_dX[i] * Cv);
			F0[tri.GetVertexIndex(i)] += f * h / m_VertexArray[tri.GetVertexIndex(i)].m_Mass;
		}
	
		// dF_dX
		// TODO:dF_dX is symmetric and below block should be modified to exploit it. 
		for ( int i = 0; i < 3; i++ )
		{
			for ( int j = 0; j < 3; j++ )
			{
				// Below line will short-circuit and save some computation. If you want to calculate dfi_dXi directly, uncomment below to lines and comment out the last line in this for block. 
				if ( i == j )
					continue;

				CMatrix33 d2Cu_dXidXj = tri.Asqrt * inv_WuLen * tri.dWu_dX[i] * tri.dWu_dX[j] * (CMatrix33::IDENTITY - tri.Wu.Out(tri.Wu));
				CMatrix33 d2Cv_dXidXj = tri.Asqrt * inv_WvLen * tri.dWv_dX[i] * tri.dWv_dX[j] * (CMatrix33::IDENTITY - tri.Wv.Out(tri.Wv));

				CMatrix33 dfi_dXj = -m_Kst * (dCu_dX[i].Out(dCu_dX[j]) + dCv_dX[i].Out(dCv_dX[j]) + d2Cu_dXidXj * Cu + d2Cv_dXidXj * Cv);

				int index_i = tri.GetVertexIndex(i);
				int index_j = tri.GetVertexIndex(j);

				CMatrix33 val = dfi_dXj * h * h / m_VertexArray[tri.GetVertexIndex(i)].m_Mass;

				dF_dX.AddToElement(index_i, index_j, val);

				//dfi_dXi = -(sum of dfi_dXj s.t j is neighbor of i)
				dF_dX.AddToElement(index_i, index_i, -val);
			}
		}

		// shear force
		double Csh = tri.Asqrt * (tri.Wu.Dot(tri.Wv));

		CVector3D dCsh_dX[3];

		for ( int i = 0; i < 3; i++ )
		{
			dCsh_dX[i] = tri.Asqrt * (tri.dWu_dX[i] * tri.Wv + tri.Wu * tri.dWv_dX[i]);

			CVector3D f = -m_Ksh * (dCsh_dX[i] * Csh);
			F0[tri.GetVertexIndex(i)] += f * h / m_VertexArray[tri.GetVertexIndex(i)].m_Mass;
		}

		// d2Csh_dXidXj
		CMatrix33 d2Csh_dXidXj;

		for ( int i = 0; i < 3; i++ )
		{
			for ( int j = 0; j < 3; j++ )
			{
				double val  = tri.Asqrt * (tri.dWu_dX[i] * tri.dWv_dX[j] + tri.dWu_dX[j] * tri.dWv_dX[i]);
				d2Csh_dXidXj.SetElement(i, j, val);
			}
		}

		// dF_dX
		// TODO:dF_dX is symmetric and below block should be modified to exploit it. 
		for ( int i = 0; i < 3; i++ )
		{
			for ( int j = 0; j < 3; j++ )
			{
				// Below line will short-circuit and save some computation. If you want to calculate dfi_dXi directly, uncomment below to lines and comment out the last line in this for block. 
				if ( i == j )
					continue;

				CMatrix33 dfi_dXj = -m_Ksh * (dCsh_dX[i].Out(dCsh_dX[j]) + d2Csh_dXidXj * Csh);

				int index_i = tri.GetVertexIndex(i);
				int index_j = tri.GetVertexIndex(j);

				CMatrix33 val = dfi_dXj * h * h / m_VertexArray[tri.GetVertexIndex(i)].m_Mass;

				dF_dX.AddToElement(index_i, index_j, val);

				//dfi_dXi = -(sum of dfi_dXj s.t j is neighbor of i)
				dF_dX.AddToElement(index_i, index_i, -val);
			}
		}
	}

	bool bImplicit = true;


	if ( bImplicit )
	{
		//A = M - dF_dV - dF_dX;
		A = I - dF_dX;
		Multiply(dF_dX, V0, temp);
		Add(F0, temp, b);

		m_NumIter = cgSolver.Solve(A, b, x, 1e-10, 500);
	}
	else
	{
		m_NumIter = cgSolver.Solve(I, F0, x);

		/*
		for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
		{
			CVertexCloth3D& vert = m_VertexArray[i];
		
			if ( vert.m_Mass > 0 )			
				vert.m_Vel += F0[vert.GetIndex()];
		}

		*/
	}

	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		CVertexCloth3D& v_i = m_VertexArray[i];

		const CVector3D& dv = x[v_i.GetIndex()];
		v_i.m_Vel += dv;
	}
}

void CCloth3D::CalcForcesByBaraff()
{
	for ( std::vector<CTriangleCloth3D>::iterator iterTri = m_TriangleArray.begin(); iterTri != m_TriangleArray.end(); ++iterTri )
	{
		CTriangleCloth3D& tri = (*iterTri);
		
		const CVector3D& p0 = m_VertexArray[tri.GetVertexIndex(0)].m_Pos;
		const CVector3D& p1 = m_VertexArray[tri.GetVertexIndex(1)].m_Pos;
		const CVector3D& p2 = m_VertexArray[tri.GetVertexIndex(2)].m_Pos;

		CVector3D dX1 = p1 - p0;
		CVector3D dX2 = p2 - p0;

		tri.Wu = tri.inv_det * (tri.dv2 * dX1 - tri.dv1 * dX2);
		tri.Wv = tri.inv_det * (-tri.du2 * dX1 + tri.du1 * dX2);

		double WuLen = tri.Wu.Length();
		double WvLen = tri.Wv.Length();
		double inv_WuLen = 1.0 / WuLen;
		double inv_WvLen = 1.0 / WvLen;

		double Cu = tri.Asqrt*(WuLen - 1.0);
		double Cv = tri.Asqrt*(WvLen - 1.0);

		CVector3D dCu_dX[3];
		CVector3D dCv_dX[3];

		// stretch force F0
		for ( int i = 0; i < 3; i++ )
		{
			dCu_dX[i] = tri.Asqrt *tri.dWu_dX[i] * tri.Wu * inv_WuLen;
			dCv_dX[i] = tri.Asqrt *tri.dWv_dX[i] * tri.Wv * inv_WvLen;

			CVector3D f = -m_Kst * (dCu_dX[i] * Cu + dCv_dX[i] * Cv);
			m_VertexArray[tri.GetVertexIndex(i)].m_Force += f;
		}
	}	
}

void CCloth3D::TranslateW(double x, double y, double z)
{
	for ( std::vector<CVertexCloth3D>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexCloth3D& vert = *iter;
				
		vert.m_Pos += CVector3D(x, y, z);	
	}

	if ( m_pBVHTree )
		m_pBVHTree->Refit();
}

CCloth3D& CCloth3D::operator=(const CCloth3D& other) 
{ 
	m_h = other.m_h;
	m_Kst = other.m_Kst;
	m_Kb = other.m_Kb;
	m_Kd = other.m_Kd;
	m_Epsilon = other.m_Epsilon;

	// ToDo: Need to do something with m_pBVHTree

	m_VertexArray = other.m_VertexArray;
	m_StrechSpringArray = other.m_StrechSpringArray;
	m_BendSpringArray = other.m_BendSpringArray;
	m_Color = other.m_Color;

	m_bDeformable = other.m_bDeformable;
	m_bEqualVertexMass = other.m_bEqualVertexMass;
	m_bShowBV = other.m_bShowBV;

	m_pBVHTree = new CBVHTree();
	m_pBVHTree->SetDeformable(m_bDeformable);
	m_pBVHTree->BuildBVH(this);

	return *this; 
}

CObstacleCloth3D::CObstacleCloth3D()
{
	m_bDeformable = false;
}

CObstacleCloth3D::CObstacleCloth3D(std::string filepath) : CCloth3D(filepath)
{
	m_bDeformable = false;
}

CObstacleCloth3D::~CObstacleCloth3D(void) 
{
}

void CObstacleCloth3D::Create(const char* filename)
{
	if ( !Load(filename) )
		return;

	// Set all mass as zero
	for ( std::vector<CVertexCloth3D>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexCloth3D& vert = *iter;
				
		vert.m_Mass = 0;	
	}

	m_bDeformable = false;

	// Build BVH tree
	m_pBVHTree = new CBVHTree();
	m_pBVHTree->SetDeformable(false);
	m_pBVHTree->BuildBVH(this);
}

void CObstacleCloth3D::Initialize()
{
	// Set all mass as zero
	for ( std::vector<CVertexCloth3D>::iterator iter = m_VertexArray.begin(); iter != m_VertexArray.end(); iter++ )
	{
		CVertexCloth3D& vert = *iter;
				
		vert.m_Mass = 0;	
	}

	m_bDeformable = false;

	// Build BVH tree
	m_pBVHTree = new CBVHTree();
	m_pBVHTree->SetDeformable(false);
	m_pBVHTree->BuildBVH(this);
}

void CObstacleCloth3D::Render()
{
	CCloth3D::Render();
}


