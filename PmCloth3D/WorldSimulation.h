#pragma once

#include "../btBulletCollisionCommon.h"

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

public:
	void Create();
	void ClearAll();
	unsigned int Update(btScalar dt);
	void Render() const;

protected:
	unsigned int SubsUpdate(btScalar dt);
	
private:
	CWorldSimulation& operator=(const CWorldSimulation& other) { return *this; }; // private assign operator. This means you cannot use assign operator.
};