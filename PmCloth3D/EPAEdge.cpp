#include <cassert>
#include "EPAEdge.h"


CEPAEdge::CEPAEdge()
{

}

CEPAEdge::CEPAEdge(CEPATriangle* pEPATriangle, int indexLocal, int indexVertex0, int indexVertex1) : m_pEPATriangle(pEPATriangle), m_IndexLocal(indexLocal) 
{ 
	assert(indexLocal >= 0 && indexLocal < 3); 

	m_IndexVertex[0] = indexVertex0;
	m_IndexVertex[1] = indexVertex1;
}

CEPAEdge::~CEPAEdge() 
{
}