#pragma once

#include <list>
#include "Aabb.h"
#include "kDOP.h"
#include "BVHTree.h"

struct BVHMasterColResult;

class CBVHMasterNode
{
	friend class CBVHMasterTree;

public:
	CBVHMasterNode(void);
	virtual ~CBVHMasterNode(void);

protected:
	CBoundingVolume m_BV;
	CBVHTree* m_pTree; // Only leaf node has a non-null value. 
	CBVHMasterNode* m_pLeftChildNode;
	CBVHMasterNode* m_pRightChildNode;

	// temporary list of CBVHTree pointers. It is only used during building tree. In normal time, it is empty.
	std::list<CBVHTree*> m_TempTreeList;

public:
	void Build();	
	void SelfCollide(std::list<BVHMasterColResult>* pResult, double tolerance = 0) const;
	IBoundingVolume& Refit();
	void ClearCollisionFlag();

	IBoundingVolume& BoundingVolume() { return m_BV; }
	const IBoundingVolume& BoundingVolume() const { return m_BV; }

	bool IsLeaf() const;

protected:
	void Collide(CBVHMasterNode& other, std::list<BVHMasterColResult>* pResult, bool bSelfCollide = false, double tolerance = 0) const;
};
