//#define _CRT_SECURE_NO_WARNINGS 

#include <common/Utils/ObjLoader.h>
#include <fstream>
#include <iostream>



ObjLoader::ObjLoader(char *filename)
{
	{
		FILE *f;
		char buf[4096];
	    
		f = std::fopen(filename, "r");

		if (!f)
		{
			printf("File is not exist");
			ADLASSERT( 0 );
		}
		
		while (fgets(buf, sizeof(buf), f))
			decode(buf);
		fclose(f);
	}

	printf("%s : %3.2fK triangles\n", filename, m_faces.getSize()/1000.f);
}

void ObjLoader::decode(char* buf)
{
    switch (buf[0]) {
        case '#':
                break;
        case 'v':
                readVertex(buf);
                break;
        case 'f':
                readFace(buf);
                break;
        default:
            break;
    }
}
void ObjLoader::readVertex(char *buf)
{
	char tmp[128];
	float4 vertex = make_float4( 1.0f );

	switch(buf[1])
	{
	case ' ':
		{
		sscanf(buf,"%s %f %f %f",tmp,&vertex.s[0], &vertex.s[1], &vertex.s[2]);
		m_vtx.pushBack(vertex);
		break;
		}
	case 'n':
		sscanf(buf,"%s %f %f %f",tmp,&vertex.s[0], &vertex.s[1], &vertex.s[2]);
		m_normals.pushBack(vertex);
		break;
	}
}
void ObjLoader::readFace(char* buf)
{
	char tmp[128];
	int nOfSlash = strchrcount(buf, '/');
	if( nOfSlash == 0 )
	{
		int4 face;
		sscanf(buf,"%s %d %d %d", tmp,&face.s[0], &face.s[1], &face.s[2]);
		face.s[0]--;face.s[1]--;face.s[2]--;
		m_faces.pushBack(face);
	}
	else if( nOfSlash == 3)
	{
		int4 face;
		int4 faceNormal;
		sscanf(buf,"%s %d/%d %d/%d %d/%d",tmp,
			&face.s[0], &faceNormal.s[0],
			&face.s[1], &faceNormal.s[1],
			&face.s[2], &faceNormal.s[2]);
		face.s[0]--;face.s[1]--;face.s[2]--;
		faceNormal.s[0]--; faceNormal.s[1]--; faceNormal.s[2]--;
		m_faces.pushBack(face);
		m_faceNormals.pushBack(faceNormal);
	}
	else if( nOfSlash == 6)
	{
		int4 face;
		int4 faceNormal;
		sscanf(buf,"%s %d//%d %d//%d %d//%d",tmp,
			&face.s[0], &faceNormal.s[0],
			&face.s[1], &faceNormal.s[1],
			&face.s[2], &faceNormal.s[2]);
		face.s[0]--;face.s[1]--;face.s[2]--;
		faceNormal.s[0]--; faceNormal.s[1]--; faceNormal.s[2]--;
		m_faces.pushBack(face);
		m_faceNormals.pushBack(faceNormal);
	}
	else
	{
		ADLASSERT( 0 );
	}
}
int ObjLoader::strchrcount(const char* str, int c)
{
	int counter=0;
	int n = (int)strlen(str);
	char* text = new char[n];
	memcpy(text, str, n*sizeof(char));
	char *ptr = text;

	while(strlen(ptr) > 0)
	{
		ptr = strchr(ptr,c);
		if(ptr == NULL)
			break;
		ptr++;
		counter++;
	}

	delete [] text;
	return counter;
}


int ObjLoader::getNumTriangles()const
{
	return m_faces.getSize();
}

int ObjLoader::getNumVertices() const
{
	return m_vtx.getSize();
}

float4* ObjLoader::getVertexBuffer()
{
	return m_vtx.begin();
}

float4* ObjLoader::getNormalPerVertexBuffer()
{
	return m_normals.begin();
}

int4* ObjLoader::getNormalIndexBuffer()
{
	return m_faceNormals.begin();
}

int4* ObjLoader::getFaceIndexBuffer()
{
	return m_faces.begin();
}

void ObjLoader::scale(const Aabb& scaleSpace, float4* vtxInOut, int numVtx)
{
	Aabb aabb;
	aabb.setEmpty();
	for(int i=0; i<numVtx; i++)
	{
		aabb.includePoint( vtxInOut[i] );
	}
	aabb.expandBy( make_float4( 0.0001f ) );

	float4 center = aabb.center();
	float4 extent = aabb.getExtent();

	float4 targetCenter = scaleSpace.center();
	float4 targetExtent = scaleSpace.getExtent();

	float scale = targetExtent.s[ scaleSpace.getMajorAxis() ] / extent.s[ aabb.getMajorAxis() ];

	for(int i=0; i<numVtx; i++)
	{
		float4& v = vtxInOut[i];
		v = scale*(v-center) + targetCenter;
	}
}

void ObjLoader::swapYZ(float4* vtxInOut, int numVtx)
{
	for(int i=0; i<numVtx; i++)
	{
		float4& v = vtxInOut[i];
		swap2( (float&)v.s[1], (float&)v.s[2] );
	}
}

void ObjLoader::translate(float4* vtxInOut, int numVtx, const float4& translate)
{
	for(int i=0; i<numVtx; i++)
	{
		float4& v = vtxInOut[i];
		v += translate;
	}
}

void ObjLoader::scale(float4* vtxInOut, int numVtx, float scale)
{
	for(int i=0; i<numVtx; i++)
	{
		float4& v = vtxInOut[i];
		v *= scale;
	}
}

void ObjLoader::flipWinding(int4* faceInOut, int numTri)
{
	for(int i=0; i<numTri; i++)
	{
		int4& f = faceInOut[i];
		swap2( f.s[1], f.s[2] );
	}
}

