#pragma once

#include "../btBulletCollisionCommon.h"
#include "ClothCL.h"
#include "ConvexHeightField\ConvexHeightFieldShape.h"
#include "ConvexHeightField\ChNarrowphase.h"
#include "global.h"

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

	float m_dt;
	int m_Substeps; // = 1
		
	//CCloth* m_pCloth;
	CVector3D m_Gravity;
	int m_RenderBatchIndex;
	bool m_bGPU; 

protected:
	Device* m_ddcl; 
	Device* m_ddhost;	
	int m_NumOfConvexRBodies;

public:
	bool InitCL();
	void Create();
	void ClearAll();
	unsigned int Update(float dt);
	void Render(bool bWireframe = false);

protected:
	unsigned int SubsUpdate(float dt);
	void ResolveCollisions(float dt);
	
private:
	CWorldSimulation& operator=(const CWorldSimulation& other) { return *this; }; // private assign operator. This means you cannot use assign operator.
};