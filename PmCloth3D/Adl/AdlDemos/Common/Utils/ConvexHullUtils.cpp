#include <common/Utils/ConvexHullUtils.h>


void ConvexHullUtils::createCvxShapeBF(const float4* points, int numPoints, Array<int4>& tris)
{
	ADLASSERT( numPoints > 3 );
	for(int i=0;i<numPoints;i++)for(int j=i+1;j<numPoints;j++)for(int k=j+1;k<numPoints;k++)
	{
		float4 v0 = points[i];
		float4 v1 = points[j];
		float4 v2 = points[k];
		float4 eqn = createEquation(v0, v1, v2);

		float minH, maxH;
		minH = maxH = 0.f;
		for(int ii=0;ii<numPoints;ii++)
		{
			if( ii==i || ii==j || ii==k ) continue;

			float h = dot3w1(points[ii], eqn);

			minH = min2( minH, h );
			maxH = max2( maxH, h );
		}
		if( minH == 0 )
		{
			int4& tri = tris.expandOne();
			tri = make_int4(i,k,j);
		}
		if( maxH == 0 )
		{
			int4& tri = tris.expandOne();
			tri = make_int4(i,j,k);
		}
	}
}

