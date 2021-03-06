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

cl_context        g_cxGPUMainContext = NULL;
cl_command_queue  g_cqGPUCommandQue = NULL;

CWorldSimulation::CWorldSimulation(void) : m_Gravity(0.0f, -9.87f, 0.0f),/* m_pCloth(NULL),*/ m_RenderBatchIndex(0), m_bGPU(false), m_bRotateObjects(false)
{ 
	m_Substeps = 1;

}

CWorldSimulation::~CWorldSimulation(void)
{
	ClearAll();
}

CCollisionObject g_MarkerA;
CCollisionObject g_MarkerB;

bool CWorldSimulation::InitCL()
{
	//------------------
	// Initialize OpenCL
	//------------------
	DeviceUtils::Config cfg;
	m_ddcl = DeviceUtils::allocate(TYPE_CL, cfg);
	m_ddhost = DeviceUtils::allocate(TYPE_HOST, cfg);

	char name[128];
	m_ddcl->getDeviceName( name );
	printf("CL: %s\n", name);

	cl_platform_id platform_id;
	cl_uint num_platforms;

	//---------
	// platform
	//---------
	cl_int err = clGetPlatformIDs(1, &platform_id, &num_platforms);

	if ( err != CL_SUCCESS )
	{
		printf("Failed to get OpenCL platform\n");
		return false;
	}

	//----------
	// device ID
	//----------
	cl_device_id device_id;
	cl_uint num_of_devices;

	err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &num_of_devices);

	if ( err != CL_SUCCESS )
	{
		printf("Failed to get OpenCL device ID\n");
		return false;
	}
	
	//clGetDeviceInfo(device_id, 

	//--------
	// context
	//--------
	cl_context_properties properties[3];

	properties[0]= CL_CONTEXT_PLATFORM;
	properties[1]= (cl_context_properties) platform_id;
	properties[2]= 0;

	g_cxGPUMainContext = clCreateContext(properties, 1, &device_id, NULL, NULL, &err);

	if ( err != CL_SUCCESS )
	{
		printf("Failed to get OpenCL context ID\n");
		return false;
	}

	//--------------
	// Command Queue
	//--------------
	g_cqGPUCommandQue = clCreateCommandQueue(g_cxGPUMainContext, device_id, 0, &err);

	if ( err != CL_SUCCESS )
	{
		printf("Failed to get OpenCL command queue ID\n");
		return false;
	}

	return true;
}

void CWorldSimulation::Create()
{	
	if ( m_bGPU )
	{
		bool bCLOk = InitCL();
		assert(bCLOk);
	}

	m_pNarrowPhase = new CNarrowPhaseCollisionDetection();
	m_pNarrowPhase->SetConvexCollisionAlgorithmType(CNarrowPhaseCollisionDetection::EMCC);

	//-----------
	// Object A
	//-----------
	if ( m_bGPU )
		pObjectA = new CCollisionObject(m_ddcl, m_ddhost);
	else
		pObjectA = new CCollisionObject();

	//pObjectA->SetCollisionObjectType(CCollisionObject::ConvexHull);
	//pObjectA->SetMargin(0.0001f); // margin should be set before Load(..) 
	//pObjectA->Load("box.obj");
	////pObjectA->Load("smallGeoSphere.obj");
	
	pObjectA->SetCollisionObjectType(CCollisionObject::Box);
	pObjectA->SetColor(0.7f, 0.7f, 0.0f);
	pObjectA->SetMargin(0.0001f);
	pObjectA->SetSize(3.0f, 1.0f, 3.0f);

	pObjectA->GetTransform().GetTranslation().Set(0.0f, 5.0f, 0.0f);
	pObjectA->SetColor(1.0f, 0.0f, 0.0f);
	//pObjectA->GetTransform().GetRotation().SetRotation(CQuaternion(CVector3D(1.0f, 1.0f, 0.0f).Normalize(), 3.141592f/3.0f));

	//-----------
	// Object B
	//-----------

	//// Line segment
	//CCollisionObject* pObjectB;

	//if ( m_bGPU )
	//	pObjectB = new CCollisionObject(m_ddcl, m_ddhost);
	//else
	//	pObjectB = new CCollisionObject();

	//pObjectB->SetCollisionObjectType(CCollisionObject::LineSegment);
	//pObjectB->SetColor(0.7f, 0.7f, 0.0f);
	//pObjectB->SetMargin(0.0f);

	//pObjectB->GetVertices().push_back(CVertex(2.0, 0.0, 0.0));
	//pObjectB->GetVertices().push_back(CVertex(-2.0, 0.0, 0.0));
	//pObjectB->GetEdges().push_back(CEdge(0, 1));
	//pObjectB->GetEdges()[0].SetIndex(0);
	//pObjectB->GetTransform().GetTranslation().Set(3.0f, 6.0f, -2.0f);

	// box convex object
	if ( m_bGPU )
		pObjectB = new CCollisionObject(m_ddcl, m_ddhost);
	else
		pObjectB = new CCollisionObject();

	// Convex Hull
	//pObjectB->SetCollisionObjectType(CCollisionObject::ConvexHull);
	//pObjectB->SetColor(0.7f, 0.7f, 0.0f);
	//pObjectB->SetMargin(0.0001f);
	//pObjectB->Load("box.obj");
	////pObjectB->Load("smallGeoSphere.obj");

	// Box shape
	pObjectB->SetCollisionObjectType(CCollisionObject::Box);
	pObjectB->SetColor(0.7f, 0.7f, 0.0f);
	pObjectB->SetMargin(0.0001f);
	pObjectB->SetSize(3.0f, 1.0f, 3.0f);

	CQuaternion rotB = CQuaternion(CVector3D(1.0f, 0.0f, 0.0f).Normalize(), 3.141592f/2.0f) * CQuaternion(CVector3D(0.0f, 1.0f, 0.0f).Normalize(), 3.141592f/4.0f);
	rotB = CQuaternion(CVector3D(0.0f, 1.0f, 0.0f).Normalize(), 3.141592f/4.0f) * rotB;

	//CQuaternion rotB(0.24053794, 0.6649446, 0.66494441, 0.24053794);
	//CQuaternion rotB(-0.34227660, -0.91814625, 0.089724503, -0.17858225);


	//pObjectB->GetTransform().GetRotation().SetRotation(rotB);
	//pObjectB->GetTransform().GetTranslation().Set(3.0f, 5.5f, 0.0f);
	pObjectB->GetTransform().GetTranslation().Set(3.0f - 1e-6, 5.5f, 1.0f);

	m_pNarrowPhase->AddPair(CNarrowCollisionInfo(pObjectA, pObjectB));

	// cloth
	//if ( m_bGPU )
	//	m_pCloth = new CClothCL();
	//else
	//	m_pCloth = new CCloth();

	////m_pCloth->Load("circle789.obj");
	////m_pCloth->Load("circle2723.obj");
	//m_pCloth->Load("circle2723.obj");
	////m_pCloth->Load("circle4074.obj");	
	//m_pCloth->SetVertexMass(1.0f);
	//m_pCloth->TranslateW(0.0f, 10.0f, 0.0f);
	//m_pCloth->SetColor(0.0f, 0.0f, 0.8f);
	//m_pCloth->SetGravity(m_Gravity);
	//m_pCloth->SetKb(10.0f);
	//m_pCloth->SetKst(100.0f); // Only meaningful when IntegrateEuler(..) is used.
	//m_pCloth->SetFrictionCoef(0.0f);
	//m_pCloth->SetNumIterForConstraintSolver(5);
	///*m_pCloth->AddPin(20);
	//m_pCloth->AddPin(500);*/
	//m_pCloth->Initialize();	

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

	/*if ( m_pCloth )
		delete m_pCloth;

	m_pCloth = NULL;*/

	if ( g_cqGPUCommandQue )
		clReleaseCommandQueue(g_cqGPUCommandQue);

	if ( g_cxGPUMainContext )
		clReleaseContext(g_cxGPUMainContext);

	m_RenderBatchIndex = 0;
	
}

unsigned int CWorldSimulation::Update(float dt)
{
	int numIter = 0;

	float sub_dt = dt / m_Substeps;
	m_dt = sub_dt;

	for ( int i = 0; i < m_Substeps; i++ )
		numIter = SubsUpdate(sub_dt);

	return numIter;
}

unsigned int CWorldSimulation::SubsUpdate(float dt)
{
	m_dt = dt;
	
	if ( m_bRotateObjects && m_pNarrowPhase && m_pNarrowPhase->GetPairs().size() > 0 )
	{
		const float angleRad = 0.001f;

		CTransform& transA = m_pNarrowPhase->GetPairs()[0].pObjA->GetTransform();
		CTransform& transB = m_pNarrowPhase->GetPairs()[0].pObjB->GetTransform();

		//----------------------------------------------------------------------------
		// Rotate object using global coordinate axes in the local coordinate system
		//----------------------------------------------------------------------------
		//transA.GetRotation() = CQuaternion(CVector3D(0.0f, 1.0f, 0.0f).Normalize(), angleRad) * transA.GetRotation();

		//----------------------------------------------------------------------------
		// Rotate object using local coordinate axes in the local coordinate system
		//----------------------------------------------------------------------------
		//transA.GetRotation() = transA.GetRotation() * CQuaternion(CVector3D(1.0f, 0.0f, 0.0f).Normalize(), angleRad);
		transB.GetRotation() = transB.GetRotation() * CQuaternion(CVector3D(0.0f, 1.0f, 0.0f).Normalize(), angleRad);

		//-----------------------------------------------------------------------
		// Translate using local coordinate axes in the local coordinate system
		//-----------------------------------------------------------------------
		//transA.GetTranslation() = transA.GetTranslation() + transA.GetRotation() * CVector3D(0.001f, 0, 0);

		//----------------------------------------------------------------------------
		// Rotate object using global coordinate axes in the global coordinate system
		//----------------------------------------------------------------------------
		//CTransform transW;
		//CQuaternion rot(CVector3D(0.0f, 1.0f, 1.0f).Normalize(), angleRad);
		//transW.GetRotation() = rot;

		///*CTransform& transA = m_pNarrowPhase->GetPairs()[0].pObjA->GetTransform();
		//transA = transW * transA;*/

		////CTransform& transB = m_pNarrowPhase->GetPairs()[0].pObjB->GetTransform();
		//transB = transW * transB;
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

	/*const CEdge& edgeB = pObjectB->GetEdges()[0];
	CVector3D edgeVert0B = pObjectB->GetTransform() * pObjectB->GetVertices()[edgeB.GetVertexIndex(0)];
	CVector3D edgeVert1B = pObjectB->GetTransform() * pObjectB->GetVertices()[edgeB.GetVertexIndex(1)];

	g_MarkerA.GetTransform().GetTranslation() = edgeVert0B;
	g_MarkerB.GetTransform().GetTranslation() = edgeVert1B;*/

	/*m_pCloth->Integrate(dt);
	m_pCloth->ResolveCollision(*pObjectA, dt);
	m_pCloth->AdvancePosition(dt);	*/

	return 0;
}

void CWorldSimulation::ResolveCollisions(float dt)
{	
	
}

void CWorldSimulation::Render(bool bWireframe/* = false*/)
{	
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1,1);

	if ( m_pNarrowPhase )
	{
		for ( int i = m_pNarrowPhase->GetPairs().size()-1; i >= 0; i-- )
		{
			//m_pNarrowPhase->GetPairs()[i].pObjA->GetTransform().GetTranslation() += (g_MarkerB.GetTransform().GetTranslation() - g_MarkerA.GetTransform().GetTranslation());
			m_pNarrowPhase->GetPairs()[i].pObjA->Render(bWireframe);
			//m_pNarrowPhase->GetPairs()[i].pObjA->GetTransform().GetTranslation() -= (g_MarkerB.GetTransform().GetTranslation() - g_MarkerA.GetTransform().GetTranslation());
			
			m_pNarrowPhase->GetPairs()[i].pObjB->Render(bWireframe);			
		}
	}

	// cloth
	/*m_pCloth->Render();
	m_RenderBatchIndex = m_pCloth->RenderBatch(m_RenderBatchIndex);*/

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

