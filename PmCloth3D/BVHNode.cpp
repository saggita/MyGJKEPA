#include <algorithm>
#include <limits>

#include "BVHNode.h"
#include "Cloth3D.h"
#include "BVHTree.h"

CBVHNode::CBVHNode(void)
{
	m_TriIndex = -1;
	m_pLeftChildNode = NULL;
	m_pRightChildNode = NULL;
	m_bCollided = false;
}

CBVHNode::~CBVHNode(void)
{
	if ( m_pLeftChildNode )
		delete m_pLeftChildNode;

	if ( m_pRightChildNode )
		delete m_pRightChildNode;
}

void CBVHNode::BuildBVH(CCloth3D* pCloth)
{
	m_pCloth = pCloth;
	m_bCollided = false;

	// compute the AABB
	m_BV.Empty();

	const std::vector<CVertexCloth3D>& vertArray = pCloth->GetVertexArray();
	const std::vector<CSpringCloth3D>& edgeArray = pCloth->GetStrechSpringArray();
	const std::vector<CTriangleCloth3D>& triArray = pCloth->GetTriangleArray();

	for ( std::list<int>::const_iterator triIter = m_TempTriIndexList.begin(); triIter != m_TempTriIndexList.end(); triIter++ )
	{
		const CTriangleCloth3D& tri = triArray[*triIter];

		// For CCD, velocity and thickness should be considered.
		for ( int i = 0; i < 3; i++ )
		{
			const CVertexCloth3D& vert = vertArray[tri.GetVertexIndex(i)];
			m_BV += vert.m_Pos;
			m_BV += vert.m_Pos + vert.m_Vel * pCloth->Getdt();
		}		
	}

	m_BV.Enlarge(pCloth->GetThickness());

	// If there is only one triangle index, this node is a leaf node. 
	if ( m_TempTriIndexList.size() == 1 )
	{
		m_TriIndex = (*m_TempTriIndexList.begin());
		m_pLeftChildNode = NULL;
		m_pRightChildNode = NULL;
		m_TempTriIndexList.clear();
		m_pTriangle = &triArray[m_TriIndex];
		return;
	}
	else if ( m_TempTriIndexList.size() == 0 )
	{
		// Must not reach here!
		assert(0);
	}

	// split recursively. 
	IBoundingVolume* leftBV;
	IBoundingVolume* rightBV;

	m_BV.Split(leftBV, rightBV);

	CBVHNode* pLeftNode = new CBVHNode();
	CBVHNode* pRightNode = new CBVHNode();

	for ( std::list<int>::const_iterator triIter = m_TempTriIndexList.begin(); triIter != m_TempTriIndexList.end(); triIter++ )
	{
		const CTriangleCloth3D& tri = triArray[*triIter];
		int index = tri.GetIndex();

		CBoundingVolume aabb;

		// For CCD, velocity and thickness should be considered.
		for ( int i = 0; i < 3; i++ )
		{
			aabb += vertArray[tri.GetVertexIndex(i)].m_Pos;
			aabb += vertArray[tri.GetVertexIndex(i)].m_Pos + vertArray[tri.GetVertexIndex(i)].m_Vel * pCloth->Getdt();
		}

		aabb.Enlarge(pCloth->GetThickness());

		if ( leftBV->Inside(aabb.Center()) )
		{
			pLeftNode->m_TempTriIndexList.push_back(index);
		}
		else
		{
			pRightNode->m_TempTriIndexList.push_back(index);
		}
	}

	delete leftBV;
	delete rightBV;

	// If all of child nodes go into one side and the other side is empty, force to split them so as to be well-balanced.
	if ( pLeftNode->m_TempTriIndexList.size() == 0 && pRightNode->m_TempTriIndexList.size() > 1 )
	{
		int idxHalf = (int)(pRightNode->m_TempTriIndexList.size() * 0.5);

		for ( int i = 0; i < idxHalf; i++ )
		{
			pLeftNode->m_TempTriIndexList.push_back(*pRightNode->m_TempTriIndexList.begin());
			pRightNode->m_TempTriIndexList.pop_front();
		}
	}
	else if ( pLeftNode->m_TempTriIndexList.size() > 1 && pRightNode->m_TempTriIndexList.size() == 0 )
	{
		int idxHalf = (int)(pLeftNode->m_TempTriIndexList.size() * 0.5);

		for ( int i = 0; i < idxHalf; i++ )
		{
			pRightNode->m_TempTriIndexList.push_back(*pLeftNode->m_TempTriIndexList.begin());
			pLeftNode->m_TempTriIndexList.pop_front();
		}
	}

	if ( pLeftNode->m_TempTriIndexList.size() > 0 )
	{
		m_pLeftChildNode = pLeftNode;
		m_pLeftChildNode->BuildBVH(pCloth);
	}
	else if ( pLeftNode->m_TempTriIndexList.size() == 0 && pRightNode->m_TempTriIndexList.size() == 1 )
	{
		delete pLeftNode;

		m_pLeftChildNode = pRightNode;
		m_pLeftChildNode->BuildBVH(pCloth);
		m_pRightChildNode = NULL;

		m_TempTriIndexList.clear();
		return;
	}
	
	if ( pRightNode->m_TempTriIndexList.size() > 0 )
	{
		m_pRightChildNode = pRightNode;
		m_pRightChildNode->BuildBVH(pCloth);
	}
	else 
		delete pRightNode;

	m_TempTriIndexList.clear();
	return;
}

IBoundingVolume& CBVHNode::Refit(const CCloth3D* pCloth)
{
	if ( IsLeaf() )
	{
		m_BV.Empty();

		const std::vector<CVertexCloth3D>& vertArray = pCloth->GetVertexArray();
		const CTriangleCloth3D& tri = pCloth->GetTriangleArray()[m_TriIndex];
		
		// For CCD, velocity and thickness should be considered.
		for ( int i = 0; i < 3; i++ )
		{
			const CVertexCloth3D& vert = vertArray[tri.GetVertexIndex(i)];

			m_BV += vert.m_Pos;
			m_BV += vert.m_Pos + vert.m_Vel * pCloth->Getdt();
		}

		m_BV.Enlarge(pCloth->GetThickness());
	}
	else
	{
		m_BV = m_pLeftChildNode->Refit(pCloth);

		if ( m_pRightChildNode )
			m_BV += m_pRightChildNode->Refit(pCloth);
	}

	return m_BV;
}

bool CBVHNode::IsLeaf() const
{
	if ( m_pLeftChildNode == NULL )
		return true;
	else
		return false;
}

void CBVHNode::Collide(CBVHNode& other, std::list<BVHCollisionResult>* pResult, bool bSelfCollide/* = false*/, double tolerance/* = 0*/) 
{
	assert(pResult);

	if ( !m_BV.Collide(other.m_BV, tolerance) )
		return;

	if ( IsLeaf() && other.IsLeaf() )
	{
		if ( bSelfCollide )
		{
			// If the two triangles are adjacent, skip checking collision.
			if ( m_pTriangle->GetVertexIndex(0) == other.m_pTriangle->GetVertexIndex(0) ||
				 m_pTriangle->GetVertexIndex(0) == other.m_pTriangle->GetVertexIndex(1) ||
				 m_pTriangle->GetVertexIndex(0) == other.m_pTriangle->GetVertexIndex(2) ||

				 m_pTriangle->GetVertexIndex(1) == other.m_pTriangle->GetVertexIndex(0) ||
				 m_pTriangle->GetVertexIndex(1) == other.m_pTriangle->GetVertexIndex(1) ||
				 m_pTriangle->GetVertexIndex(1) == other.m_pTriangle->GetVertexIndex(2) ||

				 m_pTriangle->GetVertexIndex(2) == other.m_pTriangle->GetVertexIndex(0) ||
				 m_pTriangle->GetVertexIndex(2) == other.m_pTriangle->GetVertexIndex(1) ||
				 m_pTriangle->GetVertexIndex(2) == other.m_pTriangle->GetVertexIndex(2) )
				 return;			
		}

		BVHCollisionResult colRes;

		colRes.cloths[0] = m_pCloth;
		colRes.triIndexes[0] = GetTriIndex();

		colRes.cloths[1] = other.m_pCloth;
		colRes.triIndexes[1] = other.GetTriIndex();

		pResult->push_back(colRes);
		
		m_bCollided = true;
		other.m_bCollided = true;
		return;
	}

	if ( IsLeaf() )
	{
		Collide(*other.m_pLeftChildNode, pResult, bSelfCollide, tolerance);
		Collide(*other.m_pRightChildNode, pResult, bSelfCollide, tolerance);
	}
	else
	{
		m_pLeftChildNode->Collide(other, pResult, bSelfCollide, tolerance);
		m_pRightChildNode->Collide(other, pResult, bSelfCollide, tolerance);
	}
}

void CBVHNode::SelfCollide(std::list<BVHCollisionResult>* pResult, double tolerance/* = 0*/) const
{
	assert(pResult);

	if ( IsLeaf() )
		return;

	m_pLeftChildNode->SelfCollide(pResult, tolerance);
	m_pRightChildNode->SelfCollide(pResult, tolerance);

	m_pLeftChildNode->Collide(*m_pRightChildNode, pResult, true, tolerance);
}

void CBVHNode::ClearCollisionFlag()
{
	m_bCollided = false;

	if ( m_pLeftChildNode )
		m_pLeftChildNode->ClearCollisionFlag();

	if ( m_pRightChildNode )
		m_pRightChildNode->ClearCollisionFlag();
}

void CBVHNode::Visualize(int level)
{
	/*if ( m_pLeftChildNode )
		m_pLeftChildNode->Visualize(level);

	if ( m_pRightChildNode )
		m_pRightChildNode->Visualize(level);


	if ( IsLeaf() )
		m_BV.Visualize(m_bCollided);
	
	m_bCollided = false;
	return;*/

	if ( level != -1 )
	{
		if ( IsLeaf() )
			m_BV.Visualize(true);
		else if ( level > 0 )
		{
			if ( level == 1 )
				m_BV.Visualize(true);
			else
			{
				if ( m_pLeftChildNode )
					m_pLeftChildNode->Visualize(level-1);

				if ( m_pRightChildNode )
					m_pRightChildNode->Visualize(level-1);
			}
		}
	}
	else
	{
		//if ( IsLeaf() )
			m_BV.Visualize(m_bCollided);

		if ( m_pLeftChildNode )
			m_pLeftChildNode->Visualize(level);

		if ( m_pRightChildNode )
			m_pRightChildNode->Visualize(level);
	}
}
