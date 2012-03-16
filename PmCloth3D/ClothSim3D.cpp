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

#include "ClothSim3D.h"
#include "BVHTree.h"
#include "BVHMasterTree.h"
#include "mathUtil.h"
#include "CollisionDetections.h"
#include "MINRESSolver.h"

CClothSim3D::CClothSim3D(void)
{ 
	m_Gravity= CVector3D(0, 0, -9.82);

	m_Substeps = 1;
	m_pBVHMasterTree = NULL; 
	m_BroadPhaseTolerance = 1e-8;
}

CClothSim3D::~CClothSim3D(void)
{
	ClearAll();
}

CCollisionObject g_MarkerA;
CCollisionObject g_MarkerB;

void CClothSim3D::Create()
{	
	ClearAll();

	m_pNarrorPhase = new CNarrowPhaseGJK(this);
	
	// Object 0
	CCollisionObject* pColObj = new CCollisionObject();
	pColObj->SetCollisionObjectType(CCollisionObject::Box);
	pColObj->GetTransform().GetTranslation().Set(0.0, 2.0, 0.0);
	pColObj->SetSize(6.0, 3.0, 5.0);
	pColObj->SetColor(1.0, 0.0, 0.0);
	pColObj->GetTransform().GetRotation().SetRotation(CQuaternion(CVector3D(1.0, 1.0, 0).Normalize(), 3.141592/3.0));

	// Object 1
	CCollisionObject* pPointObj = new CCollisionObject();
	pPointObj->SetCollisionObjectType(CCollisionObject::Box);
	pPointObj->SetSize(3.0, 4.0, 5.0);
	pPointObj->SetColor(0.5, 0.5, 0.5);
	pPointObj->GetTransform().GetRotation().SetRotation(CQuaternion(CVector3D(1.0, 0.0, 0).Normalize(), 0));
	pPointObj->GetTransform().GetTranslation().Set(2.0, 7.0, 0.0);
	//pPointObj->GetTransform().GetTranslation().Set(2.0, 10.0, 0.0);

	m_pNarrorPhase->m_CollisionObjectList.push_back(pColObj);
	m_pNarrorPhase->m_CollisionObjectList.push_back(pPointObj);

	g_MarkerA.SetSize(0.05, 1.5, 1.5);
	g_MarkerA.SetColor(1.0, 1.0, 0.0);
	g_MarkerB.SetSize(0.05, 1.5, 1.5);
	g_MarkerB.SetColor(1.0, 1.0, 0.0);
	g_MarkerA.SetCollisionObjectType(CCollisionObject::Sphere);
	g_MarkerB.SetCollisionObjectType(CCollisionObject::Sphere);		
	
	m_pBVHMasterTree = new CBVHMasterTree();

	for ( int i = 0; i < (int)m_ClothList.size(); i++ )
		m_pBVHMasterTree->AddBVHTree(&m_ClothList[i]->GetBVHTree());

	m_pBVHMasterTree->Build();
}

// TODO:possible memory leak or access violation. Need to do check this code..
void CClothSim3D::ClearAll()
{
	for ( unsigned int i = 0; i < m_ClothList.size(); i++ )
	{		
		if ( m_ClothList[i] )
		{
			delete m_ClothList[i];
			m_ClothList[i] = NULL;
		}
	}

	m_ClothList.clear();
	
	delete m_pBVHMasterTree;

	if ( m_pNarrorPhase )
	{
		for ( std::vector<CCollisionObject*>::iterator iter = m_pNarrorPhase->m_CollisionObjectList.begin(); iter != m_pNarrorPhase->m_CollisionObjectList.end(); iter++ )
		{
			if ( *iter )
				delete (*iter);
		}
	}

	if ( m_pNarrorPhase )
		delete m_pNarrorPhase;
}

void CClothSim3D::SetGravity(const CVector3D& gravity)
{
	m_Gravity = gravity;

	for ( unsigned int i = 0; i < m_ClothList.size(); i++ )
	{		
		m_ClothList[i]->SetGravity(m_Gravity);		
	}
}

const CVector3D& CClothSim3D:: GetGravity() const
{
	return m_Gravity;
}

void CClothSim3D::AddCloth(CCloth3D* pCloth)
{	
	m_ClothList.push_back(pCloth);

	if ( m_pBVHMasterTree )
		delete m_pBVHMasterTree;

	m_pBVHMasterTree = new CBVHMasterTree();

	for ( int i = 0; i < (int)m_ClothList.size(); i++ )
		m_pBVHMasterTree->AddBVHTree(&m_ClothList[i]->GetBVHTree());

	m_pBVHMasterTree->Build();

	return;
}

unsigned int CClothSim3D::Update(double dt)
{
	int numIter = 0;

	double sub_dt = dt / m_Substeps;
	m_dt = sub_dt;

	for ( int i = 0; i < m_Substeps; i++ )
		numIter = SubsUpdate(sub_dt);

	return numIter;
}

unsigned int CClothSim3D::SubsUpdate(double dt)
{
	m_dt = dt;

	unsigned int numIter = 0;
	m_NumOfUnresolvedCols = 0;
	m_NumOfTriTriCols = 0;
	m_NumIterForGlobalCol = 0;
	
	CNarrowCollisionInfo collisionInfo;
	bool bCollision = m_pNarrorPhase->CheckCollision(*m_pNarrorPhase->m_CollisionObjectList[0], *m_pNarrorPhase->m_CollisionObjectList[1], &collisionInfo, true);

	g_MarkerA.GetTransform().GetTranslation() = m_pNarrorPhase->m_CollisionObjectList[0]->GetTransform() * collisionInfo.witnessPntA;
	g_MarkerB.GetTransform().GetTranslation() = m_pNarrorPhase->m_CollisionObjectList[1]->GetTransform() * collisionInfo.witnessPntB;

	//double dist = (g_MarkerA.GetTransform().GetTranslation() - g_MarkerB.GetTransform().GetTranslation()).Length();

	CVector3D axis;
	static double angleRad = 0;

	angleRad += 2*3.141592 / 15000;

	if ( angleRad > 2*3.141592 )
		angleRad -= 2*3.141592;

	CMatrix33 rot;
	rot.SetRotation(CVector3D(1.0, 1.0, 1.0).Normalize(), angleRad);

	m_pNarrorPhase->m_CollisionObjectList[0]->GetTransform().GetRotation().SetRotation(CVector3D(-1.0, 1.0, 1.0).Normalize(), angleRad);
	m_pNarrorPhase->m_CollisionObjectList[1]->GetTransform().GetRotation().SetRotation(CVector3D(1.0, 1.0, 0.0).Normalize(), angleRad);
		
	return 0;
}

void CClothSim3D::Render() const
{	
	//glScalef(5.0f, 5.0f, 5.0f);
	for ( std::vector<CCloth3D*>::const_iterator iter = m_ClothList.begin(); iter != m_ClothList.end(); iter++ )
	{
		(**iter).Render();
	}

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1,1);

	/*for ( std::vector<CCollisionObject*>::const_iterator iter = m_pNarrorPhase->m_CollisionObjectList.begin(); iter != m_pNarrorPhase->m_CollisionObjectList.end(); iter++ )
	{
		(**iter).Render();
	}*/

	for ( int i = m_pNarrorPhase->m_CollisionObjectList.size()-1; i >= 0; i-- )
	{
		m_pNarrorPhase->m_CollisionObjectList[i]->Render();
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

