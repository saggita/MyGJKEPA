#pragma once


namespace adl
{
typedef void* SolverData;
typedef void* ShapeDataType;

struct Contact4
{
	_MEM_ALIGNED_ALLOCATOR16;

	float4 m_worldPos[4];
	float4 m_worldNormal;
//	float m_restituitionCoeff;
//	float m_frictionCoeff;
	u16 m_restituitionCoeffCmp;
	u16 m_frictionCoeffCmp;
	int m_batchIdx;

	u32 m_bodyAPtr;
	u32 m_bodyBPtr;

	//	todo. make it safer
	int& getBatchIdx() { return m_batchIdx; }
	float getRestituitionCoeff() const { return ((float)m_restituitionCoeffCmp/(float)0xffff); }
	void setRestituitionCoeff( float c ) { ADLASSERT( c >= 0.f && c <= 1.f ); m_restituitionCoeffCmp = (u16)(c*0xffff); }
	float getFrictionCoeff() const { return ((float)m_frictionCoeffCmp/(float)0xffff); }
	void setFrictionCoeff( float c ) { ADLASSERT( c >= 0.f && c <= 1.f ); m_frictionCoeffCmp = (u16)(c*0xffff); }

	float& getNPoints() { return m_worldNormal.w; }
	float getNPoints() const { return m_worldNormal.w; }

	float getPenetration(int idx) const { return m_worldPos[idx].w; }

	bool isInvalid() const { return ((u32)m_bodyAPtr+(u32)m_bodyBPtr) == 0; }
};

};
