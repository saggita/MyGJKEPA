#include <vector>
#include <assert.h>

//#include "Point2D.h"
//#include "Vector2D.h"

#include "BVHTree.h"

CBVHTree::CBVHTree(void) : m_pRootNode(NULL), m_bDeformable(false)
{
}

CBVHTree::~CBVHTree(void)
{
	if ( m_pRootNode )
		delete m_pRootNode;
}

void CBVHTree::BuildBVH(CCloth3D* pCloth)
{
	Clear();

	m_pCloth = pCloth;

	const std::vector<CTriangleCloth3D>& triArray = pCloth->GetTriangleArray();

	for ( std::vector<CTriangleCloth3D>::const_iterator triIter = triArray.begin(); triIter != triArray.end(); triIter++ )
	{
		const CTriangleCloth3D& tri =(*triIter);
		m_pRootNode->m_TempTriIndexList.push_back(tri.GetIndex());
	}

	if ( m_pRootNode->m_TempTriIndexList.size() > 0 )
		m_pRootNode->BuildBVH(pCloth);
}

void CBVHTree::RebuildBVH()
{
	assert(m_pCloth != NULL);
	BuildBVH(m_pCloth);
}

void CBVHTree::Clear()
{
	if ( m_pRootNode )
		delete m_pRootNode;

	m_pRootNode = new CBVHNode();
}

void CBVHTree::Collide(const CBVHTree& other, std::list<BVHCollisionResult>* pResult, double tolerance/* = 0*/) const
{
	assert(m_pRootNode);
	assert(other.m_pRootNode);
	assert(pResult);

	m_pRootNode->Collide(*other.m_pRootNode, pResult, false, tolerance);
}

void CBVHTree::SelfCollide(std::list<BVHCollisionResult>* pResult, double tolerance/* = 0*/) const
{
	assert(m_pRootNode);
	assert(pResult);

	m_pRootNode->SelfCollide(pResult, tolerance);
}

const IBoundingVolume& CBVHTree::Refit()
{
	assert(m_pRootNode != NULL);
	assert(m_pCloth);

	return m_pRootNode->Refit(m_pCloth);
}

void CBVHTree::ClearCollisionFlag()
{
	if ( m_pRootNode )
		m_pRootNode->ClearCollisionFlag();
}

void CBVHTree::Visualize(int level) const
{
	assert(m_pRootNode != NULL);

	m_pRootNode->Visualize(level);
}
