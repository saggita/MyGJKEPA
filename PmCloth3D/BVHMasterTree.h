#pragma once

#include <list>
#include "BVHTree.h"
#include "BVHMasterNode.h"

struct BVHMasterColResult
{
	CBVHTree* pBVHTree0;
	int index0; 
	
	CBVHTree* pBVHTree1;
	int index1; 
};

// CBVHMasterTree represents a BVH tree which has CBVHTree instances as leaf nodes. 
// Usually there should be only one CBVHMasterTree instance in the world scene. 
// This is the reason why there is no Collide function. SelfCollide function is sufficient. 
class CBVHMasterTree
{
public:
	CBVHMasterTree(void);
	virtual ~CBVHMasterTree(void);

	void AddBVHTree(CBVHTree* pTree);
	void Build();
	void Rebuild();
	void Clear();
	void SelfCollide(std::list<BVHMasterColResult>* pResult, double tolerance = 0) const;
	const IBoundingVolume& Refit();
	void ClearCollisionFlag();

protected:
	CBVHMasterNode* m_pRootNode; 
	std::list<CBVHTree*> m_AddedChildTreeList;
};
