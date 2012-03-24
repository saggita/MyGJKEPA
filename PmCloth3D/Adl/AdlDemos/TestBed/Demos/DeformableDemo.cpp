#include <Demos/DeformableDemo.h>
#include <common/Utils/ObjLoader.h>


void createTriangleMesh(float xSize, float ySize, int xRes, int yRes, 
	float4*& vtxOut, int2*& edges, int& nVtx, int& nEdges, int upDir = 2 )
{
	//	todo. allocate memory here
	int maxEdges = (xRes-1)*(yRes-1)*4+xRes-1+yRes-1;
	maxEdges += (xRes-2)*(yRes-2)*2;

	nVtx = xRes*yRes;

	vtxOut = new float4[nVtx];
	edges = new int2[maxEdges];

	float dx = xSize/xRes;
	float dy = ySize/yRes;

	for(int j=0; j<yRes; j++) for(int i=0; i<xRes; i++)
	{
		if( upDir == 2 )
		{
			vtxOut[i+j*xRes] = make_float4(i*dx-xSize/2.f, j*dy-ySize/2.f, 0.f);
		}
		else 
		{
			vtxOut[i+j*xRes] = make_float4(i*dx-xSize/2.f, 0.f, j*dy-ySize/2.f);
		}
	}

	nEdges = 0;
	for(int j=0; j<yRes-1; j++) for(int i=0; i<xRes-1; i++)
	{
		int a = (i)+(j)*xRes;
		int b = (i+1)+(j)*xRes;
		int c = (i+1)+(j+1)*xRes;
		int d = (i)+(j+1)*xRes;

		if( j==0 )
		{
			edges[nEdges++] = make_int2( a, b );
		}
		if( i==0 )
		{
			edges[nEdges++] = make_int2( a, d );
		}

		edges[nEdges++] = make_int2( b, c );
		edges[nEdges++] = make_int2( c, d );
		edges[nEdges++] = make_int2( a, c );
		edges[nEdges++] = make_int2( b, d );
	}

	for(int j=1; j<yRes-1; j++) for(int i=1; i<xRes-1; i++)
	{
		int xp = (i+1)+(j)*xRes;
		int xm = (i-1)+(j)*xRes;
		int yp = (i)+(j+1)*xRes;
		int ym = (i)+(j-1)*xRes;

		edges[nEdges++] = make_int2( xp, xm );
		edges[nEdges++] = make_int2( yp, ym );
	}

	ADLASSERT( nEdges == maxEdges );
}

void randomaizeEdges(int2* edges, int nEdges)
{
	for(int i=0; i<nEdges/2; i++)
	{
		int a = getRandom(0, nEdges);
		int b = getRandom(0, nEdges);

		swap2( edges[a], edges[b] );
	}
}

void extractEdges(const int4* tris, int nTris, Array<int2>& edgesOut)
{
	for(int ie=0; ie<nTris; ie++)
	{
		int4 idx = tris[ie];

		{
			if( edgesOut.indexOf( make_int2(idx.x, idx.y) ) == -1 
				&& edgesOut.indexOf( make_int2(idx.y, idx.x) ) == -1 )
			{
				edgesOut.pushBack( make_int2( idx.x, idx.y ) );
			}
			if( edgesOut.indexOf( make_int2(idx.y, idx.z) ) == -1 
				&& edgesOut.indexOf( make_int2(idx.z, idx.y) ) == -1 )
			{
				edgesOut.pushBack( make_int2( idx.y, idx.z ) );
			}
			if( edgesOut.indexOf( make_int2(idx.x, idx.z) ) == -1 
				&& edgesOut.indexOf( make_int2(idx.z, idx.x) ) == -1 )
			{
				edgesOut.pushBack( make_int2( idx.x, idx.z ) );
			}
		}
	}
}

void extractBendEdges(const float4* vtx, int nVtx,
	const int2* edges, int nEdges, Array<int2>& bendOut)
{
	for(int i=0; i<nVtx; i++)
	{
		const float4& vi = vtx[i];
		Array<float4> edgeVec;
		for(int j=0; j<nEdges; j++)
		{
			int ej = -1;
			if( edges[j].x == i ) ej = edges[j].y;
			if( edges[j].y == i ) ej = edges[j].x;

			if( ej != -1 )
			{
				float4 vij = vtx[ej] - vi;
				vij = normalize3( vij );
				vij.w = (float)ej;
				edgeVec.pushBack( vij );
			}
		}

		for(int ii=0; ii<edgeVec.getSize(); ii++) for(int jj=ii+1; jj<edgeVec.getSize(); jj++)
		{
			float4 veci = edgeVec[ii];
			float4 vecj = edgeVec[jj];

			if( dot3F4( veci, vecj ) < -1.f+FLT_EPSILON )
			{
				bendOut.pushBack( make_int2( (int)veci.w, (int)vecj.w ) );
			}
		}
	}
}

void createCloth( ObjLoader* obj, Cloth& cloth )
{
	float* mass = new float[obj->getNumVertices()];
	for(int i=0; i<obj->getNumVertices(); i++) mass[i] = 1.f;

	Array<int2> edges;
	extractEdges( obj->getFaceIndexBuffer(), obj->getNumTriangles(), edges );
		
	cloth.add( obj->getVertexBuffer(), obj->getNumVertices(), mass, edges.begin(), edges.getSize(), 0.8f );

	Array<int2> bEdges;
	extractBendEdges( obj->getVertexBuffer(), obj->getNumVertices(),
		edges.begin(), edges.getSize(), bEdges );

	cloth.add( bEdges.begin(), bEdges.getSize(), 0.3f );

	delete [] mass;
}

void collideCloth(Cloth& clothA, Cloth& clothB, float diam)
{
	float4* iVtx;
	float4* iVtxPrev;
	float4* jVtx;
	float4* jVtxPrev;

	for(iVtx=clothA.m_vtx.begin(), iVtxPrev=clothA.m_vtxPrev.begin(); iVtx<clothA.m_vtx.end(); iVtx++, iVtxPrev++)
	{
		for(jVtx=clothB.m_vtx.begin(), jVtxPrev=clothB.m_vtxPrev.begin(); jVtx<clothB.m_vtx.end(); jVtx++, jVtxPrev++)
		{
			float4 r_ij = *jVtx - *iVtx;
			float dist = length3( r_ij );

			if( dist < diam )
			{
				float4 iVel = *iVtx - *iVtxPrev;
				float4 jVel = *jVtx - *jVtxPrev;

				float penetration = diam - dist; // +
				r_ij = normalize3( r_ij );

				float4 disp = penetration*r_ij/2.f;

				*iVtx -= disp;
				*jVtx += disp;

				float4 vel = (iVel+jVel)*0.5f;

				*iVtxPrev = *iVtx - vel;
				*jVtxPrev = *jVtx - vel;
			}
		}
	}
}

DeformableDemo::DeformableDemo()
{
	m_clothSize = 0.5f;
	m_particleDiam = 0.15f;

	m_obj = new ObjLoader( "../Resources/Obj/Box5x5.obj" );

	{
		Aabb aabb; aabb.m_max = make_float4( m_clothSize ); aabb.m_min = make_float4( -m_clothSize );
		ObjLoader::scale( aabb, m_obj->getVertexBuffer(), m_obj->getNumVertices() );
	}
}

DeformableDemo::~DeformableDemo()
{
	delete m_obj;
}

void DeformableDemo::reset()
{
	m_boundary = make_float4(0,1,0,1);
	for(int i=0; i<MAX_CLOTH; i++) m_cloth[i].clear();

	if(0)
	{
		const int res = 10;

		float4* vtx;
		int2* edges;
		int nVtx, nEdges;
		createTriangleMesh( 0.75f, 0.75f, res, res, vtx, edges, nVtx, nEdges, 1 );
		float* mass = new float[nVtx];
		for(int j=0; j<res; j++)for(int i=0; i<res; i++)
		{
			int idx = i+j*res;
			mass[idx] = (j==res-1)? FLT_MAX: 1.f;
	//		mass[idx] = 1.f;
		}

	//	randomaizeEdges( edges, nEdges );

		m_cloth[0].add( vtx, nVtx, mass, edges, nEdges, 0.9f );

		delete [] vtx;
		delete [] mass;
		delete [] edges;
	}
	else
	{
		createCloth( m_obj, m_cloth[0] );

		for(int i=1; i<MAX_CLOTH; i++)
		{
			m_cloth[i].copy( m_cloth[0] );

			float4 center = make_float4( 0, m_clothSize*i*2.2f, 0 );
			for(int iv=0; iv<m_cloth[i].m_vtx.getSize(); iv++)
			{
				m_cloth[i].m_vtx[iv] += center;
				m_cloth[i].m_vtxPrev[iv] += center;
			}
		}

		if(0)
		{	//	add velocity
			float4 v = make_float4( 0.f, 0.1f, 0.f );
			for(int i=0; i<m_cloth[0].m_vtxPrev.getSize(); i++)
			{
				m_cloth[0].m_vtxPrev[i] -= v;
			}
		}
	}
}

void DeformableDemo::step(float dt)
{
	Cloth::SimCfg cfg;
	cfg.m_dt /= 2.f;
//	cfg.m_gravity = make_float4(0.f);

	float e = 0.3f;

	if(0)
	{
		int i = 0;
		m_cloth[i].step( cfg );
	}
	else
	{
		Aabb aabbs[MAX_CLOTH];

		for(int cIdx=0; cIdx<MAX_CLOTH; cIdx++)
		{
			aabbs[cIdx].setEmpty();
			for(int i=0; i<m_cloth[cIdx].m_vtx.getSize(); i++)
			{
				aabbs[cIdx].includePoint( m_cloth[cIdx].m_vtx[i] );
			}
			aabbs[cIdx].expandBy( make_float4( m_particleDiam ) );

			ClothSimulation::updateVelocity( m_cloth[cIdx].m_vtxPrev.begin(), m_cloth[cIdx].m_vtx.begin(), m_cloth[cIdx].m_vtx.getSize(), m_cloth[cIdx].m_mass.begin(), cfg.m_dt, cfg.m_gravity );
			ClothSimulation::integrate( m_cloth[cIdx].m_vtxPrev.begin(), m_cloth[cIdx].m_vtx.begin(), m_cloth[cIdx].m_vtx.getSize(), m_cloth[cIdx].m_mass.begin(), cfg.m_dt );
		}

		for(int iter=0; iter<cfg.m_nIteration; iter++)
		{
			//	inner cloth
			for(int cIdx=0; cIdx<MAX_CLOTH; cIdx++)
			{
				ClothSimulation::solve( m_cloth[cIdx].m_vtx.begin(), m_cloth[cIdx].m_links.begin(), m_cloth[cIdx].m_links.getSize() );
			}

			//	inter cloth
			{
				for(int ii=0; ii<MAX_CLOTH; ii++) for(int jj=ii+1; jj<MAX_CLOTH; jj++)
				{
					if( aabbs[ii].overlaps( aabbs[jj] ) )
						collideCloth( m_cloth[ii], m_cloth[jj], m_particleDiam );
				}
			}

			//	boundary condition
			for(int cIdx=0; cIdx<MAX_CLOTH; cIdx++)
			{
				for(int i=0; i<m_cloth[cIdx].m_vtx.getSize(); i++)
				{
					float4& vtx = m_cloth[cIdx].m_vtx[i];
					float4& vtxPrev = m_cloth[cIdx].m_vtxPrev[i];

					float h = dot3w1( vtx, m_boundary );

					if( h < 0.f )
					{
						float4 v = vtx-vtxPrev;

						vtx -= h*m_boundary;
						vtxPrev = vtx + v*e;
					}
				}
			}
		}
	}

}

void DeformableDemo::render()
{
	for(int cIdx=0; cIdx<MAX_CLOTH; cIdx++)
	{
		const Array<Cloth::Link>& links = m_cloth[cIdx].m_links;
		const Array<float4>& vtx = m_cloth[cIdx].m_vtx;

		for(int i=0; i<links.getSize(); i++)
		{
			float4 c = ( links[i].m_aWeight == 0.f && links[i].m_bWeight == 0.f ) ? make_float4(1,0,0) : make_float4(1.f);

			pxDrawLine( vtx[links[i].m_a], vtx[links[i].m_b], c );
		}
	}
}

