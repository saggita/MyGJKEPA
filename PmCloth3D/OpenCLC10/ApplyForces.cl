MSTRINGIFY(

//#pragma OPENCL EXTENSION cl_amd_printf : enable\n

// From MathCl.h
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef struct 
{
	float4 m_Pos;
	float4 m_PosNext;
	float4 m_Vel;	
	float4 m_Accel;

	float m_InvMass;
	u32 m_Index;
	u32 m_PinIndex; // if no pin is attached, -1. 
} VertexClothData;

typedef struct 
{
	u32 m_Index;
	u32 m_IndexVrx0;
	u32 m_IndexVrx1;
	float m_RestLength;
} SpringClothData;


__kernel 
void ClearForcesKernel(const u32 numVertices, const float dt, __global VertexClothData* gVertexClothData)
{
	u32 vertexID = get_global_id(0);

	if( vertexID < numVertices )
	{		
		VertexClothData vertexData = gVertexClothData[vertexID];
		vertexData.m_Accel = (float4)(0.f, 0.f, 0.f, 0.f);
		gVertexClothData[vertexID] = vertexData;
	}
}

__kernel 
void ComputeNextVertexPositionsKernel(const u32 numVertices, const float dt, __global VertexClothData* gVertexClothData)
{
	u32 vertexID = get_global_id(0);

	if( vertexID < numVertices )
	{		
		VertexClothData vertexData = gVertexClothData[vertexID];	

		//if ( vertexData.m_PinIndex >= 0 )
		//	vertexData.m_PosNext = vertexData.m_Pos;
		//else
			vertexData.m_PosNext = vertexData.m_Pos + vertexData.m_Vel * dt;
		
		gVertexClothData[vertexID] = vertexData;
	}
}

__kernel 
void ApplyGravityKernel(const u32 numVertices, const float4 gravity, const float dt, __global VertexClothData* gVertexClothData)
{
	u32 vertexID = get_global_id(0);

	if( vertexID < numVertices )
	{		
		VertexClothData vertexData = gVertexClothData[vertexID];
		vertexData.m_Accel += gravity;
		gVertexClothData[vertexID] = vertexData;
	}
}

__kernel 
void ApplyForcesKernel(const u32 numVertices, const float dt, __global VertexClothData* gVertexClothData)
{
	u32 vertexID = get_global_id(0);

	if( vertexID < numVertices )
	{		
		VertexClothData vertexData = gVertexClothData[vertexID];
		vertexData.m_Vel += vertexData.m_Accel * dt;
		gVertexClothData[vertexID] = vertexData;
	}
}

__kernel 
void EnforceEdgeConstraintsKernel(const u32 numSpringsInBatch, const u32 startSpringIndex, const float dt, __global VertexClothData* gVertexClothData, __global SpringClothData* gSpringClothData)
{
	u32 springID = get_global_id(0) + startSpringIndex;

	if( get_global_id(0) < numSpringsInBatch )
	{		
		SpringClothData springData = gSpringClothData[springID];

		u32 indexVrx0 = springData.m_IndexVrx0;
		u32 indexVrx1 = springData.m_IndexVrx1;

		VertexClothData vertexData0 = gVertexClothData[indexVrx0];
		VertexClothData vertexData1 = gVertexClothData[indexVrx1];

		float4 vrxPos0 = vertexData0.m_Pos;
		float4 vrxPos1 = vertexData1.m_Pos;

		float4 vecNewSpring = vertexData0.m_PosNext - vertexData1.m_PosNext;
		vecNewSpring.w = 0;

		float newLen = length(vecNewSpring);
		float restLen = springData.m_RestLength;

		float4 cji = (newLen-restLen)*normalize(vecNewSpring);

		float4 dVert0;
		float4 dVert1;

		dVert0 = -0.5*cji;
		dVert1 = 0.5*cji;
		
		vertexData0.m_PosNext += dVert0;
		vertexData1.m_PosNext += dVert1;
		
		gVertexClothData[indexVrx0] = vertexData0;
		gVertexClothData[indexVrx1] = vertexData1;
	}
}

__kernel 
void UpdateVelocitiesKernel(const u32 numVertices, const float dt, __global VertexClothData* gVertexClothData)
{
	u32 vertexID = get_global_id(0);

	if( vertexID < numVertices )
	{		
		VertexClothData vertexData = gVertexClothData[vertexID];
		vertexData.m_Vel = (vertexData.m_PosNext - vertexData.m_Pos)/dt;
		gVertexClothData[vertexID] = vertexData;
	}
}

__kernel 
void AdvancePositionKernel(const u32 numVertices, const float dt, __global VertexClothData* gVertexClothData)
{
	u32 vertexID = get_global_id(0);

	if( vertexID < numVertices )
	{		
		VertexClothData vertexData = gVertexClothData[vertexID];
		vertexData.m_Pos = vertexData.m_Pos + vertexData.m_Vel * dt;
		gVertexClothData[vertexID] = vertexData;
	}
}

);