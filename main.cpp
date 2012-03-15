#ifdef WIN32
#include <windows.h>
#endif

#include <GL/GL.h>
#include <GL/GLU.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <vector>
#include "HGLCamera.h"
#include "PmCloth3D\ClothSim3D.h"

#define MY_CLOTH 1 

using namespace std;

const int width = 1024, height = 800;
GLint viewport[4];
HGLCamera   hCamera(threeButtons, false, 25.0f, 0.0f, -5.0f, 30.0f, 30.0f);
GLuint btmPlate;
int mouseButton = -1;
bool bMousePressed = false;
bool bPause = false;

CClothSim3D g_Cloth;
double g_dt = 0.001;

void InitSimulation()
{
	if ( MY_CLOTH )
	{
		g_Cloth.Create();
		g_Cloth.SetGravity(CVector3D(0, -9.82, 0));
		g_Cloth.SetSubsteps(1);
	}	
}

void InitGL()
{
	// Setting Camera Moving Sensitivity..
	hCamera.SetMouseSensitivity(0.1f, 0.1f, 0.1f);

	// Generating bottom plate...
	GLfloat gray[4] = {0.5f, 0.5f, 0.5f, 1.0f};
	GLfloat black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	btmPlate = hCamera.makeBottomPlate(gray, black, 80.0f, 80.0f, 10.0f, 0.0f);	
}

void OnRender() 
{
	//glutSetWindowTitle(info);	

	static GLfloat RedSurface[]   = { 1.0f, 0.0f, 0.0f, 1.0f};
	static GLfloat GreenSurface[] = { 0.0f, 1.0f, 0.0f, 1.0f};
	static GLfloat BlueSurface[]  = { 0.0f, 0.0f, 1.0f, 1.0f};
	static GLfloat YellowSurface[]  = { 1.0f, 1.0f, 0.0f, 1.0f};

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// clear screen and depth buffer
	glLoadIdentity();										// reset modelview matrix
	
	//---------------
	//Draw background
	//---------------
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	
	glBegin(GL_QUADS);
		glColor3f(0.9F, 0.9F, 0.9F); glVertex2f( 1.0F,  1.0F);
		glColor3f(0.9F, 0.9F, 0.9F); glVertex2f(-1.0F,  1.0F);
		glColor3f(0.5F, 0.5F, 0.5F); glVertex2f(-1.0F, -1.0F);
		glColor3f(0.5F, 0.5F, 0.5F); glVertex2f( 1.0F, -1.0F);
	glEnd();
	
	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	//----------------
	// Applying camera
	//----------------
	hCamera.ApplyCamera();
	
	glCallList(btmPlate);

	// Lights
	GLfloat ambientLight[4] = {0.1f, 0.1f, 0.1f, 1.0f};
	GLfloat diffuseLight[4] = {0.7f, 0.7f, 0.7f, 1.0f};
	GLfloat lightPosition[4] = {200.0f, 200.0f, 100.0f, 0.0f};

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glEnable(GL_LIGHT0);

	//-------
	// Cloth
	//-------
	if ( MY_CLOTH )
	{
		if ( !bPause )
			g_Cloth.Update(g_dt);
	
		int NumIterGlobalCol = g_Cloth.m_NumIterForGlobalCol;
		int NumOfUnresolvedCols = g_Cloth.m_NumOfUnresolvedCols;
		int NumOfTriTriCols = g_Cloth.m_NumOfTriTriCols;

		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		g_Cloth.Render();
	}
			
	glutSwapBuffers();
}

void OnIdle()
{	
	glutPostRedisplay();	
}

void OnReshape(int w, int h) 
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (GLfloat)w / (GLfloat)h, 1.f, 300.0f);
	glMatrixMode(GL_MODELVIEW);
}

void OnMouseClick(int button, int state, int x, int y)
{
	mouseButton = button;

	if ( state == GLUT_DOWN )
	{
		bMousePressed = true;
		hCamera.mousePress(x, y);
	}
	else if ( state == GLUT_UP ) 
	{
		bMousePressed = false;
		hCamera.mouseRelease(x, y);
	}		
}

void OnMouseMove(int x, int y)
{
	if ( bMousePressed )
	{
		if( mouseButton == GLUT_LEFT_BUTTON )
		{
			hCamera.mouseMove(x, y, Left_button);
		}
		else if( mouseButton == GLUT_RIGHT_BUTTON )
		{
			hCamera.mouseMove(x, y, Right_button);
		}
		else if( mouseButton == (GLUT_LEFT_BUTTON | GLUT_RIGHT_BUTTON) )
		{
			hCamera.mouseMove(x, y, Middle_button);
		}
		else if( mouseButton == GLUT_MIDDLE_BUTTON )
		{
			hCamera.mouseMove(x, y, Middle_button);
		}	

		glutPostRedisplay();
	}
}

void OnKeyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:             // ESCAPE key
		exit(0);
		break;

	case 32: // space
		bPause = !bPause;
		break;
	}
}

void main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(width, height);
	glutCreateWindow("GJK/EPA for Cloth - Dongsoo Han");

	glutDisplayFunc(OnRender);
	glutReshapeFunc(OnReshape);
	glutIdleFunc(OnIdle);

	glutMouseFunc(OnMouseClick);
	glutMotionFunc(OnMouseMove);
	glutKeyboardFunc(OnKeyboard);

	InitSimulation();
	InitGL();

	glutMainLoop();
}

