#pragma once

#include <list>
#include "Aabb.h"
#include "kDOP.h"
#include "Cloth3D.h"

class CTriangleCloth3D;
struct BVHCollisionResult;

//typedef CAabb CBoundingVolume;
typedef CkDOP18 CBoundingVolume;

class CBVHNode
{
	friend class CBVHTree;

public:
	CBVHNode(void);
	virtual ~CBVHNode(void);
 
protected:
	CCloth3D* m_pCloth;
	CBoundingVolume m_BV;
	int m_TriIndex; // When this node is a leaf node, m_TriIndex has a meaningful value. Otherwise, it is -1.
	CBVHNode* m_pLeftChildNode;
	CBVHNode* m_pRightChildNode;
	const CTriangleCloth3D* m_pTriangle;
	std::list<int> m_TempTriIndexList; // temporary list of triangle indexes. It is only used during building tree. In normal time, it is empty.
	bool m_bCollided;

public:
	void BuildBVH(CCloth3D* pCloth);
	void Collide(CBVHNode& other, std::list<BVHCollisionResult>* pResult, bool bSelfCollide = false, double tolerance = 0) ;
	void SelfCollide(std::list<BVHCollisionResult>* pResult, double tolerance = 0) const;
	void TestPairOfPrimatives();
	IBoundingVolume& Refit(const CCloth3D* pCloth);
	void ClearCollisionFlag();

	int GetTriIndex() const { return m_TriIndex; }

	IBoundingVolume& BoundingVolume() { return m_BV; }
	const IBoundingVolume& BoundingVolume() const { return m_BV; }

	bool IsLeaf() const;
	void Visualize(int level) ;
};
