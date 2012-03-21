#pragma once

#include "NarrowPhaseGJK.h"


class CClothSim3D
{
public:
	
	CClothSim3D(void);
	virtual ~CClothSim3D(void);

private:
	CClothSim3D(const CClothSim3D& other) {}; // private copy constructor

public:
	CNarrowPhaseGJK* m_pNarrorPhase;

	double m_dt;
	int m_Substeps; // = 1

public:
	void Create();
	void ClearAll();
	unsigned int Update(double dt);
	void Render() const;

protected:
	unsigned int SubsUpdate(double dt);
	
private:
	CClothSim3D& operator=(const CClothSim3D& other) { return *this; }; // private assign operator. This means you cannot use assign operator.
};