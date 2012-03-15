#pragma once

#include "Cloth3D.h"
#include "BVHNode.h"

struct BVHCollisionResult
{
	CCloth3D* cloths[2];
	int triIndexes[2];
};

class CBVHTree
{
public:
	CBVHTree(void);
	virtual ~CBVHTree(void);

private:
	CCloth3D* m_pCloth;
	CBVHNode* m_pRootNode;
	bool m_bDeformable; /* = false */
	double m_CollisionTolerance;

public:
	void BuildBVH(CCloth3D* m_pCloth);
	void RebuildBVH();
	void Clear();
	void Collide(const CBVHTree& other, std::list<BVHCollisionResult>* pResult, double tolerance = 0) const;
	void SelfCollide(std::list<BVHCollisionResult>* pResult, double tolerance = 0) const;
	const IBoundingVolume& Refit();
	void ClearCollisionFlag();
	void Visualize(int level) const;
	const CBVHNode& GetRootNode() const { return *m_pRootNode; }
	bool IsDeformable() const { return m_bDeformable; }
	void SetDeformable(bool bDeformable) { m_bDeformable = bDeformable; }
};
