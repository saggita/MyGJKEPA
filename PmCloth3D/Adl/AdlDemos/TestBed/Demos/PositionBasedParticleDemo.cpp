#include <Demos/PositionBasedParticleDemo.h>

PositionBasedParticleDemo::PositionBasedParticleDemo()
	: Demo()
{
	m_pos = new float4[NUM_PARTICLES];
	m_posPrev = new float4[NUM_PARTICLES];

	m_radius = 0.05f;
	m_boundary[0] = make_float4(0,1,0,1);
	m_boundary[1] = make_float4(1,0,0,1);
	m_boundary[2] = make_float4(-1,0,0,1);


	m_nIter = 40;
}

PositionBasedParticleDemo::~PositionBasedParticleDemo()
{
	delete [] m_pos;
	delete [] m_posPrev;
}

void PositionBasedParticleDemo::reset()
{
	const int width = (int)(2.f/(m_radius*4.f));

	for(int i=0; i<NUM_PARTICLES; i++)
	{
		int x = i%width;
		int y = i/width;

//		m_pos[i] = make_float4( 0.f, i*m_radius*3.f, 0.f );
		m_pos[i] = make_float4( (x-width/2)*m_radius*3.f, y*m_radius*3.f, 0.f );
		m_posPrev[i] = m_pos[i];
	}

//	if(0)
	{
		float4 v = make_float4( 0.1f, -0.1f, 0.f );
		m_posPrev[0] -= v;
	}
}

void PositionBasedParticleDemo::solve()
{
	const float e = 0.4f;
	const float pRelaxCoeff = 0.95f;
	const float pError = m_radius*0.01f;

//	for(int iter=0; iter<4; iter++)
	{
		for(int ip=0; ip<m_pairs.getSize(); ip++)
//		for(int i=0; i<NUM_PARTICLES; i++) for(int j=i+1; j<NUM_PARTICLES; j++)
		{
			int i = m_pairs[ip].x;
			int j = m_pairs[ip].y;

			float4& ri = m_pos[i];
			float4& rj = m_pos[j];

			float4 rij = rj - ri;
			float dist = length3( rij );

			float penetration = (2*m_radius-pError) - dist;

			if( penetration > 0.f )
			{
				float4& vi = m_pos[i] - m_posPrev[i];
				float4& vj = m_pos[j] - m_posPrev[j];
				float4 vij = vj - vi;

				rij = normalize3( rij );

				float4 disp = penetration*rij/2.f * pRelaxCoeff;
				ri -= disp;
				rj += disp;

				float dist1 = length3( rj-ri );

				dist1 += 0.f;

				//	removing this condition improves
//				if( dot3F4( rij, vij ) < 0.f )
				{
					float4 vii = vi;
					float4 vjj = vj;
					if( dot3F4( rij, vij ) < 0.f )
					{
						vii = ((1-e)*vi+(1+e)*vj)/2.f;
						vjj = ((1+e)*vi+(1-e)*vj)/2.f;
					}

					float c = 0.1f;

					vii -= c*penetration*rij;
					vjj += c*penetration*rij;

					m_posPrev[i] = ri - vii;
					m_posPrev[j] = rj - vjj;
				}

//				if( iter==0 ) m_pairs.pushBack( make_int2( i, j ) );
			}
		}

		for(int ib=0; ib<3; ib++)
		{
			for(int i=0; i<NUM_PARTICLES; i++)
			{
				float4& ri = m_pos[i];
				float4 rij = -m_boundary[ib];
				float dist = dot3w1( ri, m_boundary[ib] );
				float penetration = m_radius- pError - dist;

				if( penetration > 0.f )
				{
					float4& vi = m_pos[i] - m_posPrev[i];
					float4 vij = dot3F4( vi, m_boundary[ib] ) * m_boundary[ib];
					float4 disp = penetration*rij * pRelaxCoeff;
					ri -= disp;

					if( dot3F4( rij, -vi ) < 0.f )
					{
						float4 vs = vi - vij;
						float4 vii = vi - (1+e)*vij - vs*0.1f;

						m_posPrev[i] = ri - vii;
					}			
				}
			}
		}
	}
}

void PositionBasedParticleDemo::step(float dt)
{
	m_pairs.clear();
	dt *= 0.5f;

	const float4 g = make_float4(0.f, -9.8f, 0.f);
//	const float4 g = make_float4(0.f, 0.f, 0.f);

//	if(0)
	for(int i=0; i<NUM_PARTICLES; i++)
	{
		m_posPrev[i] -= g*dt;
		float4 v = m_pos[i] - m_posPrev[i];
		m_posPrev[i] = m_pos[i];
		m_pos[i] += v;
	}

	{	// find pairs
		const float rangeCoeff = 1.5f;
		for(int i=0; i<NUM_PARTICLES; i++) for(int j=i+1; j<NUM_PARTICLES; j++)
		{
			float4& ri = m_pos[i];
			float4& rj = m_pos[j];

			float4 rij = rj - ri;
			float dist = length3( rij );

			if( dist < 2*m_radius*rangeCoeff )
			{
				m_pairs.pushBack( make_int2( i, j ) );
			}
		}
	}

	for(int iter=0; iter<m_nIter; iter++)
	{
		solve();
	}


	if(0)
	{
		float4 v = m_pos[5] - m_posPrev[5];
		adlDebugPrintf("%3.2f, %3.2f, %3.2f\n", v.x, v.y, v.z);
		for(int i=0; i<m_pairs.getSize(); i++)
		{
			if( m_pairs[i].x == 5 || m_pairs[i].y == 5 ) adlDebugPrintf("(%d, %d)", m_pairs[i].x, m_pairs[i].y);
		}
		adlDebugPrintf("\n");
	}

	{
		m_nTxtLines = 0;
		sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "%d", m_nIter);
	}
}

void PositionBasedParticleDemo::render()
{
	float4 r[] = {make_float4(m_radius, 0, 0), make_float4(0, m_radius, 0)};

	for(int i=0; i<NUM_PARTICLES; i++)
	{
		const float4& p = m_pos[i];
		pxDrawLine( p, m_posPrev[i], make_float4(0,1,0) );

		float4 c = make_float4(1.f);
		if( i==5 ) c = make_float4(0,0,1);
		pxDrawLine( p-r[0], p+r[0], c );
		pxDrawLine( p-r[1], p+r[1], c );
	}

	if(0)
	for(int i=0; i<m_pairs.getSize(); i++)
	{

		pxDrawLine( m_pos[m_pairs[i].x], m_pos[m_pairs[i].y], make_float4(1,0,0) );
	}
}

void PositionBasedParticleDemo::keyListener(unsigned char key)
{
	switch(key)
	{
	case 'I':
		solve();
		break;
	case 'W':
		m_nIter++;
		break;
	case 'S':
		m_nIter--;
		break;
	default:
		break;
	};
}
