#include "BVHMasterTree.h"

CBVHMasterTree::CBVHMasterTree(void)
{
	m_pRootNode = new CBVHMasterNode();
}

CBVHMasterTree::~CBVHMasterTree(void)
{
	if ( m_pRootNode )
	{
		delete m_pRootNode;
		m_pRootNode = NULL;
	}
}

void CBVHMasterTree::AddBVHTree(CBVHTree* pTree)
{
	assert(m_pRootNode); 
	assert(pTree);

	m_pRootNode->m_TempTreeList.push_back(pTree);
	m_AddedChildTreeList.push_back(pTree);
}

void CBVHMasterTree::Build()
{
	assert(m_pRootNode); 

	if ( m_pRootNode->m_TempTreeList.size() > 0 )
		m_pRootNode->Build();
}

void CBVHMasterTree::Rebuild()
{
	CBVHMasterNode* pTemp = m_pRootNode;
	CBVHMasterNode* pNewRootNode = new CBVHMasterNode();

	pNewRootNode->m_TempTreeList = m_AddedChildTreeList;

	for ( std::list<CBVHTree*>::iterator iter = m_AddedChildTreeList.begin(); iter != m_AddedChildTreeList.end(); iter++ )
	{
		(*iter)->RebuildBVH();
	}

	if ( m_pRootNode )
		delete m_pRootNode;

	m_pRootNode = pNewRootNode;

	Build();
}

void CBVHMasterTree::Clear()
{
	if ( m_pRootNode )
		delete m_pRootNode;

	m_pRootNode = new CBVHMasterNode();
}

void CBVHMasterTree::SelfCollide(std::list<BVHMasterColResult>* pResult, double tolerance/*= 0*/) const
{
	assert(m_pRootNode);  
	m_pRootNode->SelfCollide(pResult, tolerance);
}

const IBoundingVolume& CBVHMasterTree::Refit()
{
	assert(m_pRootNode); 

	return m_pRootNode->Refit();
}

void CBVHMasterTree::ClearCollisionFlag()
{
	if ( m_pRootNode )
		m_pRootNode->ClearCollisionFlag();
}