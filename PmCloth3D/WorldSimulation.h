#pragma once

#include "../btBulletCollisionCommon.h"
#include "Cloth3D.h"

class CNarrowPhaseCollisionDetection;

class CWorldSimulation
{
public:
	
	CWorldSimulation(void);
	virtual ~CWorldSimulation(void);

private:
	CWorldSimulation(const CWorldSimulation& other) {}; // private copy constructor

public:
	CNarrowPhaseCollisionDetection* m_pNarrowPhase;

	btScalar m_dt;
	int m_Substeps; // = 1

	CCollisionObject* pObjectA;
	CCloth3D m_Cloth;
	std::vector<CCollisionObject*> clothVertices;
	CVector3D m_Gravity;

public:
	void Create();
	void ClearAll();
	unsigned int Update(btScalar dt);
	void Render(bool bWireframe = false);

protected:
	unsigned int SubsUpdate(btScalar dt);
	void ResolveCollisions(btScalar dt);
	
private:
	CWorldSimulation& operator=(const CWorldSimulation& other) { return *this; }; // private assign operator. This means you cannot use assign operator.
};