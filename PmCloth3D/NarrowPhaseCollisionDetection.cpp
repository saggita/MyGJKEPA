#include "NarrowPhaseCollisionDetection.h"
#include "GJKAlgorithm.h"
#include "BIMAlgorithm.h"
#include "CHFAlgorithm.h"

CNarrowPhaseCollisionDetection::CNarrowPhaseCollisionDetection(void)
{
	//m_pAlgorithm = new CGJKAlgorithm;
	//m_pAlgorithm = new CBIMAlgorithm;
	m_pAlgorithm = new CCHFAlgorithm;
}


CNarrowPhaseCollisionDetection::~CNarrowPhaseCollisionDetection(void)
{
	delete m_pAlgorithm;
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