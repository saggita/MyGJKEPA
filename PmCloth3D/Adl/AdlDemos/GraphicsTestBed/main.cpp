#define _CRT_SECURE_NO_WARNINGS 
#define GLUT_DISABLE_ATEXIT_HACK

#include <AdlGraphics/DeviceDraw.h>
#include <AdlGraphics/BmpUtils.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
//#include <Common/Base/Error.h>

#pragma warning( disable : 4996 )


//#define CONSOLE


//#define INIT_HEIGHT 240
//#define INIT_WIDTH 320

//#define INIT_HEIGHT 480
//#define INIT_WIDTH 640

#define INIT_HEIGHT 720
#define INIT_WIDTH 1024

//#define INIT_HEIGHT 1920
//#define INIT_WIDTH 1920
//1280

int g_wWidth,g_wHeight;
bool g_fullScreen = false;
bool g_autoMove = false;
int g_mouseX, g_mouseY;
int g_mouseStatus = 0;
float g_rotationX = 0.0, g_rotationY = 0.0;
float g_translationX = 0.0f;
float g_translationY = 0.0f;
float g_translationZ = 0.0f;

float g_fov = 35.f;

bool g_writeBmp = 0;
int g_bmpCount = 0;

#include <omp.h>


#include <GDemos/OcclusionCullingDemo.h>
#include <GDemos/SimpleDeferredDemo.h>
#include <GDemos/ClusteredDeferredDemo.h>
#include <GDemos/CullingDebugDemo.h>


#include <Demos/MixedParticlesDemo.h>


typedef Demo* CreateFunc(const Device* deviceData);

CreateFunc* createFuncs[] = {
	MixedParticlesDemo::createFunc,

	CullingDebugDemo::createFunc, 
//	ClusteredDeferredDemo::createFunc,
//	SimpleDeferredDemo::createFunc,
//	OcclusionCullingDemo::createFunc,

};


Demo* g_demo;

void writeToBmp(char* filename);

void drawDemo()
{
	g_demo->render();
}

void step()
{
	if( g_demo->m_stepCount == -153 )
	{
		g_autoMove = false;
	}

	g_demo->stepDemo();

#if !defined(CONSOLE)
	if( g_writeBmp ) g_demo->stepDemo();

	if( g_writeBmp )
	{
		char filename[256];
		sprintf_s( filename, 256, "../Bmp/file%05d.bmp", g_bmpCount );
		writeToBmp( filename );
		g_bmpCount++;
	}
#endif
}

void initDemo( Device* deviceData = NULL )
{
	g_demo = NULL;
	g_demo = createFuncs[0]( deviceData );

	ADLASSERT( g_demo );
	g_demo->init();
	g_demo->reset();
}

void resetDemo()
{
	g_demo->resetDemo();
}

void finishDemo()
{
	if( g_demo ) delete g_demo;
	g_demo = 0;
}

void keyListenerDemo(unsigned char key)
{
	g_demo->keyListener(key);
}

void keySpecialListenerDemo(int key, int x, int y)
{
	g_demo->keySpecialListener(key);
}

//---

#if defined(CONSOLE)
int main(int argc, char *argv[])
{
	initDemo();
	step();
	finishDemo();

	return 0;
}
#else

#if !defined(DX11RENDER)

#include <glut.h>
//#pragma comment(lib,"glut32.lib")

bool drawText = true;

void setModelViewMatrix()
{
	glLoadIdentity();
	glTranslatef(g_translationX, g_translationY, g_translationZ);
	glRotatef(g_rotationX, 1.0, 0.0, 0.0);
	glRotatef(g_rotationY, 0.0, 1.0, 0.0);
}
void setPerspectiveProjMatrix()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(g_fov, (double)g_wWidth / (double)g_wHeight, 0.001, 100.0);
	gluLookAt(.0, .0, 4.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	glMatrixMode(GL_MODELVIEW);
}
void setOrthoProjMatrix()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1,1,-1,1,-100,100);
	glMatrixMode(GL_MODELVIEW);
}
void drawDemo2D()
{
//	glColor3f(1,1,1);

	float spacing = 0.04f*INIT_HEIGHT/g_wHeight;
	float4 pos;
	pos.x = pos.y = pos.z = 0.f;

	pos.x = -1.0f+spacing;
	pos.y = 1.0f-spacing;

	for(int i=0; i<g_demo->m_nTxtLines; i++)
	{
		pxDrawText( g_demo->m_txtBuffer[i], pos );
		pos.y -= spacing;
	}
}
void display(void)
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	if( g_demo->m_enableLighting )
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}
	else
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	}

	glLoadIdentity();
	{
		setPerspectiveProjMatrix();
		setModelViewMatrix();
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glColor3f(1, 1, 1);

	if( g_demo->m_enableLighting )
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

		glEnable(GL_COLOR_MATERIAL);
		glEnable(GL_NORMALIZE);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		float lightPos[] = {0,2,2,1};
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
		float s = 0.3f;
		float ambColor[] = {s,s,s,1.f};
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambColor);
	}
	else
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	}

	drawDemo();

	if( g_demo->m_enableLighting )
	{
		glDisable(GL_LIGHT0);
		glDisable(GL_LIGHTING);
	}

	if( drawText )
	{
		setOrthoProjMatrix();
		glLoadIdentity();
		drawDemo2D();
	}

	glutSwapBuffers();
}
void idle(void)
{
	if( g_autoMove )
		step();

	glutPostRedisplay();
}
void resize(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1,1,-1,1,-100,100);
	gluLookAt(.0, .0, 4.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	glMatrixMode(GL_MODELVIEW);
	g_wWidth=w;g_wHeight=h;
}
void init()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);

	initDemo();
	

	{
//		char txt[128];
//		sprintf(txt,"Dem: %dK(%d)", m_pSim->m_numParticles/1024, m_pSim->m_numParticles);
//		glutSetWindowTitle(txt);
	}
}

void mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN) {
		g_mouseStatus |= 1<<button;
	} else if (state == GLUT_UP) {
		g_mouseStatus = 0;
	}
	if (glutGetModifiers() & GLUT_ACTIVE_SHIFT
		 && state == GLUT_DOWN){
		g_mouseStatus |= 2 << 2;
	}
	
	g_mouseX = x;
	g_mouseY = y;
	glutPostRedisplay();
}
void motion(int x, int y)
{
	float dx, dy;
	dx = x - g_mouseX;
	dy = y - g_mouseY;
	
	if(g_mouseStatus & (2 << 2) && g_mouseStatus & 1){
		
	}else if (g_mouseStatus & 1) {
		g_rotationX += dy * 0.2;
		g_rotationY += dx * 0.2;
	} else if (g_mouseStatus & (2<<2)) {
		g_translationZ += dy * 0.01;
	} else if (g_mouseStatus & 4){
		g_translationX += dx * 0.005;
		g_translationY -= dy * 0.005;
	}
	
	
	g_mouseX = x;
	g_mouseY = y;
}
void keyboard(unsigned char key, int x, int y)
{
	float dz = 0.1f;
	const char str = key;

	keyListenerDemo(key);
	
	switch (key) {
		case 'a':
			g_autoMove = !g_autoMove;
			break;
		case 'r':
			resetDemo();
			break;
		case' ':
			step();
			break;
		case 't':
			drawText = !drawText;
			break;
		case 'l':
			g_demo->m_enableLighting = !g_demo->m_enableLighting;
			break;
		case 'q':
		case 'Q':
		case '\033':
			finishDemo();
			exit(0);
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			break;
		default:
			break;
	}
	glutPostRedisplay();
}
int main(int argc, char *argv[])
{
	glutInitWindowSize(INIT_WIDTH,INIT_HEIGHT);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA| GLUT_DEPTH | GLUT_DOUBLE | GLUT_ALPHA);

	if( g_fullScreen )
	{
		glutEnterGameMode();
	}
	else
	{
		glutCreateWindow(argv[0]);
	}

	glutDisplayFunc(display);
	glutReshapeFunc(resize);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(keySpecialListenerDemo);
	glutIdleFunc(idle);
	init();
	glutMainLoop();
	
	return 0;
}

void writeToBmp(char* filename)
{

}

void registerPick(float4* pos, float rad)
{

}

#else

#include <AdlGraphics/AdlGraphics.h>
#include <AdlGraphics/AdlGraphics.inl>

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	adlGraphics.allocate(hInstance, hPrevInstance, lpCmdLine, nCmdShow, INIT_WIDTH, INIT_HEIGHT);

	initDemo( g_deviceData );

	adlGraphics.renderLoop();

	adlGraphics.deallocate();

    return 0;
}

#endif

#endif
