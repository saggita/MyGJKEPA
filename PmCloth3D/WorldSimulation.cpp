#include <omp.h>

#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN
#  undef NOMINMAX
#endif

#include <GL/gl.h>
#include <map>
#include <algorithm>

#include "WorldSimulation.h"
#include "mathUtil.h"
#include "NarrowPhaseCollisionDetection.h"
#include "CollisionObject.h"

CWorldSimulation::CWorldSimulation(void)
{ 
	m_Substeps = 1;
}

CWorldSimulation::~CWorldSimulation(void)
{
	ClearAll();
}

CCollisionObject g_MarkerA;
CCollisionObject g_MarkerB;

void CWorldSimulation::Create()
{	
	ClearAll();

	m_pNarrowPhase = new CNarrowPhaseCollisionDetection();
	
	// Object 0
	CCollisionObject* pObjectA = new CCollisionObject();
	pObjectA->SetCollisionObjectType(CCollisionObject::ConvexHull);
	pObjectA->GetTransform().GetTranslation().Set(2.0, 4.6, 0.0);
	pObjectA->SetSize(6.0, 3.0, 5.0);
	pObjectA->SetColor(1.0f, 0.0f, 0.0f);
	pObjectA->GetTransform().GetRotation().SetRotation(CQuaternion(CVector3D(1.0, 1.0, 0).Normalize(), 3.141592/3.0));

	// Object 1
	CCollisionObject* pObjectB = new CCollisionObject();
	pObjectB->SetCollisionObjectType(CCollisionObject::Point);
	pObjectB->SetSize(3.0, 4.0, 5.0);
	pObjectB->SetColor(0.7f, 0.7f, 0.0f);
	pObjectB->GetTransform().GetRotation().SetRotation(CQuaternion(CVector3D(1.0, 0.0, 0).Normalize(), 0));
	pObjectB->GetTransform().GetTranslation().Set(2.0, 12.0, 0.0);
	//pObjectB->GetTransform().GetTranslation().Set(2.0, 10.0, 0.0);

	m_pNarrowPhase->AddPair(CNarrowCollisionInfo(pObjectA, pObjectB));

	g_MarkerA.SetSize(0.05, 1.5, 1.5);
	g_MarkerA.SetColor(1.0, 1.0, 0.0);
	g_MarkerB.SetSize(0.05, 1.5, 1.5);
	g_MarkerB.SetColor(1.0, 1.0, 0.0);
	g_MarkerA.SetCollisionObjectType(CCollisionObject::Sphere);
	g_MarkerB.SetCollisionObjectType(CCollisionObject::Sphere);		
}

void CWorldSimulation::ClearAll()
{
	if ( m_pNarrowPhase )
	{
		for ( std::vector<CNarrowCollisionInfo>::iterator iter = m_pNarrowPhase->GetPairs().begin(); iter != m_pNarrowPhase->GetPairs().end(); iter++ )
		{
			delete (*iter).pObjA;
			delete (*iter).pObjB;
		}
	}

	if ( m_pNarrowPhase )
		delete m_pNarrowPhase;
}

unsigned int CWorldSimulation::Update(double dt)
{
	int numIter = 0;

	double sub_dt = dt / m_Substeps;
	m_dt = sub_dt;

	for ( int i = 0; i < m_Substeps; i++ )
		numIter = SubsUpdate(sub_dt);

	return numIter;
}

unsigned int CWorldSimulation::SubsUpdate(double dt)
{
	m_dt = dt;

	unsigned int numIter = 0;

	CVector3D axis;
	static double angleRad = 0;

	angleRad += 2*3.141592 / 15000;

	if ( angleRad > 2*3.141592 )
		angleRad -= 2*3.141592;

	CMatrix33 rot;
	rot.SetRotation(CVector3D(1.0, 1.0, 1.0).Normalize(), angleRad);

	if ( m_pNarrowPhase && m_pNarrowPhase->GetPairs().size() > 0 )
	{
		m_pNarrowPhase->GetPairs()[0].pObjA->GetTransform().GetRotation().SetRotation(CVector3D(-1.0, 1.0, 1.0).Normalize(), angleRad);
		m_pNarrowPhase->GetPairs()[0].pObjB->GetTransform().GetRotation().SetRotation(CVector3D(1.0, 1.0, 0.0).Normalize(), angleRad);
	}

	m_pNarrowPhase->CheckCollisions();

	if ( m_pNarrowPhase && m_pNarrowPhase->GetPairs().size() > 0 )
	{
		if ( m_pNarrowPhase->GetPairs()[0].bIntersect == true || m_pNarrowPhase->GetPairs()[0].proximityDistance > 0 )
		{
			g_MarkerA.GetTransform().GetTranslation() = m_pNarrowPhase->GetPairs()[0].pObjA->GetTransform() * m_pNarrowPhase->GetPairs()[0].witnessPntA;
			g_MarkerB.GetTransform().GetTranslation() = m_pNarrowPhase->GetPairs()[0].pObjB->GetTransform() * m_pNarrowPhase->GetPairs()[0].witnessPntB;
		}
		else
		{
			g_MarkerA.GetTransform().GetTranslation().Set(0, 0, 0);
			g_MarkerB.GetTransform().GetTranslation().Set(0, 0, 0);
		}
	}

	return 0;
}

void CWorldSimulation::Render() const
{	
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1,1);

	if ( m_pNarrowPhase )
	{
		for ( int i = m_pNarrowPhase->GetPairs().size()-1; i >= 0; i-- )
		{
			m_pNarrowPhase->GetPairs()[i].pObjA->Render();
			m_pNarrowPhase->GetPairs()[i].pObjB->Render();
		}
	}

	glDisable(GL_DEPTH_TEST);

	glLineWidth(3.0f);
	glDisable(GL_LIGHT0);
	glColor3f(0,1,0);
	glBegin(GL_LINES);
	glVertex3d(g_MarkerA.GetTransform().GetTranslation().m_X, g_MarkerA.GetTransform().GetTranslation().m_Y, g_MarkerA.GetTransform().GetTranslation().m_Z);
	glVertex3d(g_MarkerB.GetTransform().GetTranslation().m_X, g_MarkerB.GetTransform().GetTranslation().m_Y, g_MarkerB.GetTransform().GetTranslation().m_Z);
	glEnd();
		
	glEnable(GL_LIGHT0);
	g_MarkerA.Render();
	g_MarkerB.Render();
}

