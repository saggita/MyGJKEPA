#ifndef CLOTH_SIMULATION_H
#define CLOTH_SIMULATION_H

//#include <Common/Math/Math.h>
//#include <Common/Math/Array.h>
#include <AdlPrimitives/Math/Math.h>
#include <AdlPrimitives/Math/Array.h>

class ClothSimulation
{
	public:
		struct Link
		{
			Link(){}
			Link(u32 aIdx, u32 bIdx) : m_a(aIdx), m_b(bIdx), m_dampingFactor(0.f) {}

			bool operator == (const Link& link) const { return m_a == link.m_a && m_b == link.m_b; }

			u32 m_a;
			u32 m_b;
			float m_restLength;
			float m_aWeight;
			float m_bWeight;
			float m_dampingFactor;
		};

		static
		Link createLink(const float4& posA, const float4& posB, u32 idxA, u32 idxB, float massA, float massB, float dampingFactor = 0.f);

		static
		Link createLink(const float4* vtx, const float* mass, u32 idxA, u32 idxB, float dampingFactor = 0.f);

		static
		void solve( float4* vtxBuffer, const Link& link );

		static 
		void solve( float4* vtxBuffer, Link* links, int nLinks );

		static
		void updateVelocity( float4* vtxPrev, float4* vtxCurrent, int nVtx, float* mass, float dt, const float4& g );

		static
		void integrate( float4* vtxPrev, float4* vtxCurrent, int nVtx, float* mass, float dt );

		static
		float calcVolume( const float4* vtx, const int4* tris, int nTris );

		//	volume constraint isn't symmetric. So it can create motion. 
		static
		void volumeConstraint( float4* vtx, float* mass, const int4* tris, int nTris, float initVolume, float coeff );

		static
		int extractEdges( int4* tris, int nTris, int2* edgeOut )
		{
			int nEdges = 0;
			for(int i=0; i<nTris; i++)
			{
				int4& iTri = tris[i];
				bool found[3] = {false, false, false};

				for(int j=0; j<nEdges; j++)
				{
					if( ( iTri.x == edgeOut[j].x && iTri.y == edgeOut[j].y ) || ( iTri.x == edgeOut[j].y && iTri.y == edgeOut[j].x ) )
						found[0] = true;
					if( ( iTri.y == edgeOut[j].x && iTri.z == edgeOut[j].y ) || ( iTri.y == edgeOut[j].y && iTri.z == edgeOut[j].x ) )
						found[1] = true;
					if( ( iTri.z == edgeOut[j].x && iTri.x == edgeOut[j].y ) || ( iTri.z == edgeOut[j].y && iTri.x == edgeOut[j].x ) )
						found[2] = true;
					if( found[0] && found[1] && found[2] ) break;
				}

				if( !found[0] ) edgeOut[nEdges++] = make_int2( iTri.x, iTri.y );
				if( !found[1] ) edgeOut[nEdges++] = make_int2( iTri.y, iTri.z );
				if( !found[2] ) edgeOut[nEdges++] = make_int2( iTri.z, iTri.x );
			}
			return nEdges;
		}

	public:

};


class Cloth
{
	public:
		typedef ClothSimulation::Link Link;
		struct SimCfg
		{
			SimCfg(): m_dt(1.f/60.f), m_nIteration(3) { m_gravity = make_float4(0,-9.8f,0);}

			float4 m_gravity;
			float m_dt;
			int m_nIteration;
		};

		void copy(const Cloth& cloth);

		void add(const float4* vtx, int nVtx, float* vtxMass,
			const int2* edges, int nEdges, float dampingFactor = 0.f);

		void add(const int2* edges, int nEdges, float dampingFactor = 0.f);

		void addVolume(const float4* vtx, int nVtx, float* vtxMass,
			const int2* edges, int nEdges, 
			const int4* tris, int nTris, float dampingFactor = 0.f, 
			float volumeTarget = 1.f, float volumeFactor = 0.01f);

		void clear();

		void step( const SimCfg& cfg );

	public:
		struct VConstraintData
		{
			VConstraintData(){}
			VConstraintData(int idx, int n, float v, float f):
				m_startIdx(idx), m_nIdx(n), m_volume(v), m_factor(f){}

			int m_startIdx;
			int m_nIdx;
			float m_volume;
			float m_factor;
		};

		Array<float4> m_vtx;
		Array<float4> m_vtxPrev;
		Array<Link> m_links;
		Array<float> m_mass;

		//	for volume constraint
		Array<VConstraintData> m_vData;
		Array<int4> m_vTris;
};



#endif
