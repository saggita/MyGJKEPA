#pragma once

#include "../btBulletCollisionCommon.h"
#include "ClothCL.h"
#include "ConvexHeightField\ConvexHeightFieldShape.h"
#include "ConvexHeightField\ChNarrowphase.h"

extern cl_context        g_cxGPUMainContext;
extern cl_command_queue  g_cqGPUCommandQue;

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
	CCloth m_Cloth;
	std::vector<CCollisionObject*> clothVertices;
	CVector3D m_Gravity;

protected:
	Device* m_ddcl; 
	Device* m_ddhost;	
	int m_NumOfConvexRBodies;

public:
	bool InitCL();
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