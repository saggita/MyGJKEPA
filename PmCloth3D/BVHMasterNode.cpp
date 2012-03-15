#include "BVHMasterNode.h"
#include "BVHMasterTree.h"

CBVHMasterNode::CBVHMasterNode(void) : m_pTree(NULL), m_pLeftChildNode(NULL), m_pRightChildNode(NULL)
{
}

CBVHMasterNode::~CBVHMasterNode(void)
{
	if ( m_pLeftChildNode )
		delete m_pLeftChildNode;

	if ( m_pRightChildNode )
		delete m_pRightChildNode;
}

void CBVHMasterNode::Build()
{
	m_pTree = NULL;

	// compute the AABB
	m_BV.Empty();

	for ( std::list<CBVHTree*>::const_iterator iter = m_TempTreeList.begin(); iter != m_TempTreeList.end(); iter++ )
	{
		m_BV += (*iter)->GetRootNode().BoundingVolume();
	}

	// This is a leaf node
	if ( m_TempTreeList.size() == 1 )
	{
		m_pTree = (*m_TempTreeList.begin());
		m_pLeftChildNode = m_pRightChildNode = NULL;
		m_TempTreeList.clear();
		return;
	}
	else if ( m_TempTreeList.size() == 0 )
	{
		// Must not reach here!
		assert(0);
	}

	// split recursively. 
	IBoundingVolume* leftBV;
	IBoundingVolume* rightBV;

	m_BV.Split(leftBV, rightBV);

	CBVHMasterNode* pLeftNode = new CBVHMasterNode();
	CBVHMasterNode* pRightNode = new CBVHMasterNode();

	for ( std::list<CBVHTree*>::const_iterator iter = m_TempTreeList.begin(); iter != m_TempTreeList.end(); iter++ )
	{
		CBVHTree* pTree = (*iter);

		if ( leftBV->Inside(pTree->GetRootNode().BoundingVolume().Center()) )
			pLeftNode->m_TempTreeList.push_back(pTree);
		else
			pRightNode->m_TempTreeList.push_back(pTree);
	}

	delete leftBV;
	delete rightBV;

	// If all of child nodes go into one side and the other side is empty, force to split them so as to be well-balanced.
	if ( pLeftNode->m_TempTreeList.size() == 0 && pRightNode->m_TempTreeList.size() > 1 )
	{
		int idxHalf = (int)(pRightNode->m_TempTreeList.size() * 0.5);

		for ( int i = 0; i < idxHalf; i++ )
		{
			pLeftNode->m_TempTreeList.push_back(*pRightNode->m_TempTreeList.begin());
			pRightNode->m_TempTreeList.pop_front();
		}
	}
	else if ( pLeftNode->m_TempTreeList.size() > 1 && pRightNode->m_TempTreeList.size() == 0 )
	{
		int idxHalf = (int)(pLeftNode->m_TempTreeList.size() * 0.5);

		for ( int i = 0; i < idxHalf; i++ )
		{
			pRightNode->m_TempTreeList.push_back(*pLeftNode->m_TempTreeList.begin());
			pLeftNode->m_TempTreeList.pop_front();
		}
	}

	if ( pLeftNode->m_TempTreeList.size() > 0 )
	{
		m_pLeftChildNode = pLeftNode;
		m_pLeftChildNode->Build();
	}
	else if ( pLeftNode->m_TempTreeList.size() == 0 && pRightNode->m_TempTreeList.size() == 1 )
	{
		delete pLeftNode;

		m_pLeftChildNode = pRightNode;
		m_pLeftChildNode->Build();
		m_pRightChildNode = NULL;

		m_TempTreeList.clear();
		return;
	}

	if ( pRightNode->m_TempTreeList.size() > 0 )
	{
		m_pRightChildNode = pRightNode;
		m_pRightChildNode->Build();
	}
	else
		delete pRightNode;

	m_TempTreeList.clear();
	return;
}

void CBVHMasterNode::SelfCollide(std::list<BVHMasterColResult>* pResult, double tolerance/* = 0*/) const
{
	assert(pResult);

	if ( !m_pTree )
		return;

	if ( IsLeaf() )
		return;

	m_pLeftChildNode->SelfCollide(pResult, tolerance);
	m_pRightChildNode->SelfCollide(pResult, tolerance);

	m_pLeftChildNode->Collide(*m_pRightChildNode, pResult, true, tolerance);
}

IBoundingVolume& CBVHMasterNode::Refit()
{
	if ( !m_pTree )
		return m_BV;

	if ( IsLeaf() )
	{
		if ( m_pTree->IsDeformable() )
			m_BV = m_pTree->Refit();
		else
			m_BV = m_pTree->GetRootNode().BoundingVolume();
	}
	else
	{
		m_BV = m_pLeftChildNode->Refit();

		if ( m_pRightChildNode )
			m_BV += m_pRightChildNode->Refit();
	}

	return m_BV;	
}

void CBVHMasterNode::ClearCollisionFlag()
{
	if ( !m_pTree )
		return;

	if ( IsLeaf() )
		m_pTree->ClearCollisionFlag();

	if ( m_pLeftChildNode )
		m_pLeftChildNode->ClearCollisionFlag();

	if ( m_pRightChildNode )
		m_pRightChildNode->ClearCollisionFlag();
}

bool CBVHMasterNode::IsLeaf() const
{
	if ( m_pLeftChildNode == NULL )
	{
		assert(m_pTree != NULL);
		return true;
	}
	else
	{
		assert(m_pTree == NULL);
		return false;
	}
}

void CBVHMasterNode::Collide(CBVHMasterNode& other, std::list<BVHMasterColResult>* pResult, bool bSelfCollide/* = false*/, double tolerance/* = 0*/) const
{
	assert(pResult);

	if ( !m_BV.Collide(other.m_BV, tolerance) )
		return;

	if ( IsLeaf() && other.IsLeaf() )
	{
		BVHMasterColResult colRes;
		colRes.pBVHTree0 = m_pTree;
		colRes.pBVHTree1 = other.m_pTree;
		pResult->push_back(colRes);
		
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