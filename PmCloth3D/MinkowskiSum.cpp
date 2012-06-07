#include "MinkowskiSum.h"


CMinkowskiSum::CMinkowskiSum(void)
{
}


CMinkowskiSum::~CMinkowskiSum(void)
{
}

bool CMinkowskiSum::Create(const CCollisionObject& objA, const CCollisionObject& objB)
{
	CTransform transA2W = objA.GetTransform();

	CVector3D point = transA2W * objB.GetTransform().GetTranslation();


	assert(objB.GetEdges().size() > 0 );

	const CEdge& edge = objB.GetEdges()[0];


	for ( int i = 0; i < (int)objA.GetFaces().size(); i++ )
	{
		const CTriangleFace& tri = objA.GetFaces()[i];
		
		
	}

	return true;
}

void CMinkowskiSum::Render(bool bWireframe/* = false*/) const
{

}