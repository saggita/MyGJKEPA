#include "NarrowPhaseCollisionDetection.h"
#include "ConvexCollisionAlgorithm.h"
#include "GJKAlgorithm.h"
#include "BIMAlgorithm.h"
#include "CHFAlgorithm.h"

CNarrowPhaseCollisionDetection::CNarrowPhaseCollisionDetection(void) : m_AlgorithmType(GJK_EPA), m_pAlgorithm(NULL)
{
	SetConvexCollisionAlgorithm(m_AlgorithmType);
}

CNarrowPhaseCollisionDetection::~CNarrowPhaseCollisionDetection(void)
{
	delete m_pAlgorithm;
}

void CNarrowPhaseCollisionDetection::SetConvexCollisionAlgorithm(CollisionAlgorithmType algorithmType) 
{ 
	if ( m_pAlgorithm )
		delete m_pAlgorithm;

	m_AlgorithmType = algorithmType; 

	if ( m_AlgorithmType == GJK_EPA )
		m_pAlgorithm = new CGJKAlgorithm;
	else if ( m_AlgorithmType == BIM )
		m_pAlgorithm = new CBIMAlgorithm;
	else if ( m_AlgorithmType == CHF )
		m_pAlgorithm = new CCHFAlgorithm;
}

int CNarrowPhaseCollisionDetection::CheckCollisions()
{
	if ( !m_pAlgorithm )
		return -1;

	int numIntersections = 0;

	for ( std::vector<CNarrowCollisionInfo>::iterator iter = m_CollisionPairs.begin(); iter != m_CollisionPairs.end(); iter++ )
	{
		if ( m_pAlgorithm->CheckCollision(*(*iter).pObjA, *(*iter).pObjB, &(*iter), true) )
			++numIntersections;
	}

	return numIntersections;
}