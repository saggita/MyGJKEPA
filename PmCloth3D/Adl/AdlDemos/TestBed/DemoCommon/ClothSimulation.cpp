#include <TestBed/DemoCommon/ClothSimulation.h>
#include <AdlPrimitives/Math/Matrix3x3.h>



ClothSimulation::Link ClothSimulation::createLink(const float4& posA, const float4& posB, u32 idxA, u32 idxB, float massA, float massB, float dampingFactor )
{
	Link link( idxA, idxB );
	link.m_dampingFactor = dampingFactor;
	link.m_restLength = length3( posB-posA );

	if( massA == FLT_MAX && massB == FLT_MAX )
	{
		link.m_aWeight = 0;
		link.m_bWeight = 0;
	}
	else if( massA == FLT_MAX )
	{
		link.m_aWeight = 0;
		link.m_bWeight = 1;
	}
	else if( massB == FLT_MAX )
	{
		link.m_aWeight = 1;
		link.m_bWeight = 0;
	}
	else
	{
		float massAB = massA + massB;
		link.m_aWeight = massA/massAB;
		link.m_bWeight = massB/massAB;
	}
	return link;
}

ClothSimulation::Link ClothSimulation::createLink(const float4* vtx, const float* mass, u32 idxA, u32 idxB, float dampingFactor)
{
	return createLink( vtx[idxA], vtx[idxB], idxA, idxB, mass[idxA], mass[idxB], dampingFactor );
}

void ClothSimulation::solve( float4* v, const Link& link )
{
	float4 ab = v[link.m_b] - v[link.m_a];
	float stretch = length3( ab ) - link.m_restLength;
	stretch *= (1.f-link.m_dampingFactor);
	float4 dx = stretch * normalize3( ab );
	v[link.m_a] += dx*link.m_aWeight;
	v[link.m_b] -= dx*link.m_bWeight;
}

void ClothSimulation::solve( float4 *vtxBuffer, ClothSimulation::Link *links, int nLinks )
{
	for(int i=0; i<nLinks; i++)
	{
		solve( vtxBuffer, links[i] );
	}
}

void ClothSimulation::updateVelocity( float4* vtxPrev, float4* vtxCurrent, int nVtx, float* mass, float dt, const float4& g )
{
	for(int i=0; i<nVtx; i++)
	{
		if( mass[i] == FLT_MAX ) continue;

		vtxPrev[i] -= g*dt*dt*0.5f;
	}
}

void ClothSimulation::integrate( float4* vtxPrev, float4* vtxCurrent, int nVtx, float* mass, float dt )
{
	for(int i=0; i<nVtx; i++)
	{
		if( mass[i] == FLT_MAX ) continue;

		float4 p = vtxPrev[i];
		float4 c = vtxCurrent[i];
		float4 d = (c-p);
		vtxCurrent[i] += d;
		vtxPrev[i] = c;
	}
}

float ClothSimulation::calcVolume( const float4* v, const int4* t, int n )
{
	float vol = 0;
	for(int i=0; i<n; i++)
	{
		const int4& tri = t[i];
		float4 c = cross3(v[tri.y], v[tri.x]);
		vol += dot3F4( c, v[tri.z] );
	}
	return fabs( vol/6.f );
}

void ClothSimulation::volumeConstraint( float4* vtx, float* mass, const int4* tris, int nTris, float initVolume, float coeff )
{
	float vol = calcVolume( vtx, tris, nTris );

	float force = (vol - initVolume)*coeff;

	int nVtx = 0;
	for(int i=0; i<nTris; i++)
	{
		nVtx = max2( nVtx, max2( tris[i].x, max2( tris[i].y, tris[i].z) ) );
	}
	nVtx += 1;

	float4* disp = new float4[nVtx];
	for(int i=0; i<nVtx; i++) disp[i] = make_float4(0,0,0,0);

	for(int i=0; i<nTris; i++)
	{
		const int4& t = tris[i];
		float4& v0 = vtx[t.x];
		float4& v1 = vtx[t.y];
		float4& v2 = vtx[t.z];

		const float4 n = cross3(v1-v0, v2-v0 );
		float nl = length3( n );
		float area = nl/2.f;

//		force = max2( 0.f, force );
		force = min2( 0.f, force );

		float4 f = force/3.f*(n/nl);
		if( mass[t.x] != FLT_MAX )
			disp[t.x] += f;
//			v0 += f;
		if( mass[t.y] != FLT_MAX )
			disp[t.y] += f;
//			v1 += f;
		if( mass[t.z] != FLT_MAX )
			disp[t.z] += f;
//			v2 += f;
	}

	for(int i=0; i<nVtx; i++)
		vtx[i] += disp[i];

	delete [] disp;
}


void Cloth::copy( const Cloth& cloth )
{
	m_vtx.setSize( cloth.m_vtx.getSize() );
	m_vtxPrev.setSize( cloth.m_vtxPrev.getSize() );
	m_links.setSize( cloth.m_links.getSize() );
	m_mass.setSize( cloth.m_mass.getSize() );

	m_vData.setSize( cloth.m_vData.getSize() );
	m_vTris.setSize( cloth.m_vTris.getSize() );

	memcpy( m_vtx.begin(), cloth.m_vtx.begin(), sizeof(float4)*m_vtx.getSize() );
	memcpy( m_vtxPrev.begin(), cloth.m_vtxPrev.begin(), sizeof(float4)*m_vtxPrev.getSize() );
	memcpy( m_links.begin(), cloth.m_links.begin(), sizeof(Link)*m_links.getSize() );
	memcpy( m_mass.begin(), cloth.m_mass.begin(), sizeof(float)*m_mass.getSize() );

	memcpy( m_vData.begin(), cloth.m_vData.begin(), sizeof(VConstraintData)*m_vData.getSize() );
	memcpy( m_vTris.begin(), cloth.m_vTris.begin(), sizeof(int4)*m_vTris.getSize() );
}

void Cloth::add(const float4* vtx, int nVtx, float* mass,
				const int2* edges, int nEdges, float dampingFactor)
{
	int vtxOffset = m_vtx.getSize();
	int linkOffset = m_links.getSize();

	m_vtx.setSize( vtxOffset+nVtx );
	memcpy( m_vtx.begin()+vtxOffset, vtx, sizeof(float4)*nVtx );
	m_vtxPrev.setSize( vtxOffset+nVtx );
	memcpy( m_vtxPrev.begin()+vtxOffset, vtx, sizeof(float4)*nVtx );

	m_mass.setSize( vtxOffset+nVtx );
	memcpy( m_mass.begin()+vtxOffset, mass, sizeof(float)*nVtx );

	m_links.setSize( linkOffset+nEdges );
	for(int i=0; i<nEdges; i++)
	{
		int a = edges[i].x;
		int b = edges[i].y;
		m_links[linkOffset+i] = ClothSimulation::createLink(m_vtx.begin(), m_mass.begin(), 
			a+vtxOffset, b+vtxOffset, dampingFactor);
	}
}

void Cloth::add(const int2* edges, int nEdges, float dampingFactor)
{
	int linkOffset = m_links.getSize();

	m_links.setSize( linkOffset+nEdges );
	for(int i=0; i<nEdges; i++)
	{
		int a = edges[i].x;
		int b = edges[i].y;
		m_links[linkOffset+i] = ClothSimulation::createLink(m_vtx.begin(), m_mass.begin(), 
			a, b, dampingFactor);
	}
}

void Cloth::addVolume(const float4* vtx, int nVtx, float* vtxMass,
					  const int2* edges, int nEdges, 
					  const int4* tris, int nTris, float dampingFactor,
					  float volumeTarget, float volumeFactor)
{
	int vtxOffset = m_vtx.getSize();

	int triOffset = m_vTris.getSize();

	m_vTris.setSize( triOffset+nTris );
	for(int i=0; i<nTris; i++)
	{
		m_vTris[triOffset+i] = make_int4(tris[i].x+vtxOffset, 
			tris[i].y+vtxOffset, 
			tris[i].z+vtxOffset, 0);
	}

	m_vData.pushBack( VConstraintData(triOffset, nTris, 
		ClothSimulation::calcVolume( vtx, tris, nTris )*volumeTarget, volumeFactor) );

	add( vtx, nVtx, vtxMass, edges, nEdges, dampingFactor );
}

void Cloth::clear()
{
	m_vtx.clear();
	m_vtxPrev.clear();
	m_links.clear();
	m_mass.clear();
}

void calcRigidMotion(const float4* vtx, const float4* vtxPrev, int nVtx, float4& v, float4& w, float4& c)
{
	float4 ang;
	Matrix3x3 inertia;
	c = make_float4(0);
	v = make_float4(0);
	ang = make_float4(0);
	inertia = mtIdentity();
	for(int i=0; i<nVtx; i++)
	{
		c += vtx[i];
		v += vtx[i]-vtxPrev[i];
	}
	c /= (float)nVtx;
	v /= (float)nVtx;
	for(int i=0; i<nVtx; i++)
	{
		const float4& r = vtx[i]-c;
		ang += cross3( r, vtx[i]-vtxPrev[i] );
		Matrix3x3 m;
		m.m_row[0] = make_float4(r.y*r.y + r.z*r.z, -r.x*r.y, -r.x*r.z);
		m.m_row[1] = make_float4(-r.y*r.x, r.x*r.x + r.z*r.z, -r.y*r.z);
		m.m_row[2] = make_float4(-r.z*r.x, -r.z*r.y, r.x*r.x + r.y*r.y);
		inertia = inertia + m;
	}
	Matrix3x3 invInertia = mtInvert( inertia );
	w = mtMul1( invInertia, ang );
}

void Cloth::step( const SimCfg& cfg )
{
	ClothSimulation::updateVelocity( m_vtxPrev.begin(), m_vtx.begin(), m_vtx.getSize(), m_mass.begin(), cfg.m_dt, cfg.m_gravity );
	ClothSimulation::integrate( m_vtxPrev.begin(), m_vtx.begin(), m_vtx.getSize(), m_mass.begin(), cfg.m_dt );

	for(int i=0; i<cfg.m_nIteration; i++)
	{
		for(int iv=0; iv<m_vData.getSize(); iv++)
		{
			VConstraintData& data = m_vData[iv];
			ClothSimulation::volumeConstraint( m_vtx.begin(), m_mass.begin(), 
				m_vTris.begin() + data.m_startIdx, data.m_nIdx, data.m_volume, data.m_factor );
		}
		ClothSimulation::solve( m_vtx.begin(), m_links.begin(), m_links.getSize() );

		//for(int iv=0; iv<m_vtx.getSize(); iv++)
		//{
		//	if( m_vtx[iv].y < -0.5f )
		//	{
		//		m_vtx[iv].y = -0.5f;
		//		m_vtxPrev[iv] = m_vtx[iv];
		//	}
		//}
if(0)
		{
			float4 v,w,c;
			calcRigidMotion( m_vtx.begin(), m_vtxPrev.begin(), m_vtx.getSize(), v, w, c );
			v *= 0.95f;
			w *= 0.05f;
			for(int iv=0; iv<m_vtx.getSize(); iv++)
			{
				float4 cv = (m_vtx[i]-m_vtxPrev[i]);
				float4 dv = v - cv;
//				float4 dv = v+cross3(w, m_vtx[i]-c) - cv;
//				cv += 1.f*dv;
				cv = v;
				m_vtxPrev[i] = m_vtx[i]-cv;
			}
		}
	}
}
