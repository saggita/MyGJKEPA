#pragma once

#include <CL\cl.h>
#include "Cloth.h"

class CClothCL : public CCloth
{
public:
	CClothCL(void);
	virtual ~CClothCL(void);

protected:
	cl_mem m_VertexArrayCL;
	cl_mem m_StrechSpringArrayCL;
	cl_mem m_BendSpringArrayCL;

public:
	virtual void Initialize();

	virtual void IntegrateByLocalPositionContraints(btScalar dt);
	virtual void IntegrateEuler(btScalar dt);
	virtual void AdvancePosition(btScalar dt);
};

