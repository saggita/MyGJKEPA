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
#include "ConvexCollisionAlgorithm.h"
#include "CollisionObject.h"

CWorldSimulation::CWorldSimulation(void) : m_Gravity(0.0f, -9.87f, 0.0f)
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
	pObjectA = new CCollisionObject();
	pObjectA->SetCollisionObjectType(CCollisionObject::ConvexHull);
	pObjectA->Load("smallGeoSphere.obj");
	//pObjectA->Load("cone.obj");
	//pObjectA->Load("box.obj");
	//pObjectA->Load("cylinder.obj");
	pObjectA->GetTransform().GetTranslation().Set(2.0f, 4.6f, 0.0f);
	pObjectA->SetSize(6.0f, 3.0f, 5.0f);
	pObjectA->SetColor(1.0f, 0.0f, 0.0f);
	pObjectA->GetTransform().GetRotation().SetRotation(CQuaternion(CVector3D(1.0f, 1.0f, 0.0f).Normalize(), 3.141592f/3.0f));

	// Object 1
	CCollisionObject* pObjectB = new CCollisionObject();
	pObjectB->SetCollisionObjectType(CCollisionObject::Point);
	pObjectB->SetSize(3.0f, 4.0f, 5.0f);
	pObjectB->SetColor(0.7f, 0.7f, 0.0f);
	pObjectB->GetTransform().GetRotation().SetRotation(CQuaternion(CVector3D(1.0f, 0.0f, 0.0f).Normalize(), 0.0f));
	pObjectB->GetTransform().GetTranslation().Set(2.0f, 5.5f, 0.0f);
	//pObjectB->GetTransform().GetTranslation().Set(2.0f, 10.0f, 0.0f);

	m_pNarrowPhase->AddPair(CNarrowCollisionInfo(pObjectA, pObjectB));

	// cloth
	m_Cloth.Load("circle789.obj");
	m_Cloth.SetVertexMass(0.01f);
	m_Cloth.TranslateW(0.0f, 10.0f, 0.0f);
	m_Cloth.SetColor(0.0f, 0.0f, 0.8f);
	m_Cloth.SetGravity(m_Gravity);
	m_Cloth.SetKb(10.0f);
	m_Cloth.SetNumIterForConstraintSolver(1);

	clothVertices.reserve(m_Cloth.GetVertexArray().size());

	for( int i=0; i < m_Cloth.GetVertexArray().size(); i++ ) 
	{
		const CVector3D& p = m_Cloth.GetVertexArray()[i].m_Pos;
	
		CCollisionObject* pObj = new CCollisionObject();
		pObj->SetCollisionObjectType(CCollisionObject::Point);
		pObj->GetTransform().GetTranslation() = p;
		pObj->SetColor(1.0, 1.0, 0.0);
		clothVertices.push_back(pObj);
	}

	// markers
	g_MarkerA.SetSize(0.05f, 1.5f, 1.5f);
	g_MarkerA.SetColor(1.0f, 1.0f, 0.0f);
	g_MarkerB.SetSize(0.05f, 1.5f, 1.5f);
	g_MarkerB.SetColor(1.0f, 1.0f, 0.0f);
	g_MarkerA.SetCollisionObjectType(CCollisionObject::Sphere);
	g_MarkerB.SetCollisionObjectType(CCollisionObject::Sphere);		
}

void CWorldSimulation::ClearAll()
{
	// TODO: Need to come up with a better idea since ICollidable was introduced..

	/*if ( m_pNarrowPhase )
	{
		for ( std::vector<CNarrowCollisionInfo>::iterator iter = m_pNarrowPhase->GetPairs().begin(); iter != m_pNarrowPhase->GetPairs().end(); iter++ )
		{
			delete (*iter).pObjA;
			delete (*iter).pObjB;
		}
	}*/

	if ( m_pNarrowPhase )
		delete m_pNarrowPhase;

	m_pNarrowPhase = NULL;

	for ( int i = 0; i < (int)clothVertices.size(); i++ )
		delete clothVertices[i];

	clothVertices.clear();
}

unsigned int CWorldSimulation::Update(btScalar dt)
{
	int numIter = 0;

	btScalar sub_dt = dt / m_Substeps;
	m_dt = sub_dt;

	for ( int i = 0; i < m_Substeps; i++ )
		numIter = SubsUpdate(sub_dt);

	return numIter;
}

unsigned int CWorldSimulation::SubsUpdate(btScalar dt)
{
	m_dt = dt;
		
	if ( m_pNarrowPhase && m_pNarrowPhase->GetPairs().size() > 0 )
	{
		const btScalar angleRad = 0.001f;

		CTransform& transA = m_pNarrowPhase->GetPairs()[0].pObjA->GetTransform();
		CTransform& transB = m_pNarrowPhase->GetPairs()[0].pObjB->GetTransform();

		//----------------------------------------------------------------------------
		// Rotate object using global coordinate axes in the local coordinate system
		//----------------------------------------------------------------------------
		//transA.GetRotation() = CQuaternion(CVector3D(0.0f, 1.0f, 0.0f).Normalize(), angleRad) * transA.GetRotation();

		//----------------------------------------------------------------------------
		// Rotate object using local coordinate axes in the local coordinate system
		//----------------------------------------------------------------------------
		transA.GetRotation() = transA.GetRotation() * CQuaternion(CVector3D(1.0f, 0.0f, 0.0f).Normalize(), angleRad);
		transB.GetRotation() = transB.GetRotation() * CQuaternion(CVector3D(1.0f, 0.0f, 0.0f).Normalize(), angleRad);

		//-----------------------------------------------------------------------
		// Translate using local coordinate axes in the local coordinate system
		//-----------------------------------------------------------------------
		//transA.GetTranslation() = transA.GetTranslation() + transA.GetRotation() * CVector3D(0.001f, 0, 0);

		//----------------------------------------------------------------------------
		// Rotate object using global coordinate axes in the global coordinate system
		//----------------------------------------------------------------------------
		/*CTransform transW;
		CQuaternion rot(CVector3D(0.0f, 1.0f, 1.0f).Normalize(), angleRad);
		transW.GetRotation() = rot;

		CTransform& transA = m_pNarrowPhase->GetPairs()[0].pObjA->GetTransform();
		transA = transW * transA;*/

		/*CTransform& transB = m_pNarrowPhase->GetPairs()[0].pObjB->GetTransform();
		transB = transW * transB;*/
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

	// Cloth vs convex object
	m_Cloth.IntegrateByLocalPositionContraints(m_dt);
	for ( int i = 0; i < (int)clothVertices.size(); i++ )
	{
		CNarrowCollisionInfo info;
		
		if ( m_pNarrowPhase->GetConvexCollisionAlgorithm()->CheckCollision(*pObjectA, *clothVertices[i], &info, false) )
		{
			CVector3D pointAW = pObjectA->GetTransform() * info.witnessPntA;
			CVector3D pointBW = m_Cloth.GetVertexArray()[i].m_Pos;

			CVector3D v = pointBW - pointAW;
			btScalar margin = 0.2;

			m_Cloth.GetVertexArray()[i].m_Pos -= v.Normalize() * (info.penetrationDepth + margin);
			//m_Cloth.GetVertexArray()[i].m_Vel.Set(0, 0, 0);
		}
	}

	m_Cloth.AdvancePosition(m_dt);

	for ( int i=0; i< (int)clothVertices.size();i++) 
	{
		clothVertices[i]->GetTransform().GetTranslation() = m_Cloth.GetVertexArray()[i].m_Pos;
	}

	return 0;
}

void CWorldSimulation::Render()
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

	// cloth
	m_Cloth.Render();

	for ( int i = 0; i < (int)clothVertices.size(); i++ )
		clothVertices[i]->Render();

	// Markers
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

