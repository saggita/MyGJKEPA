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
#include "PmCloth3D\WorldSimulation.h"
#include "PmCloth3D\NarrowPhaseCollisionDetection.h"

using namespace std;

const int width = 1024, height = 800;
GLint viewport[4];
HGLCamera   hCamera(threeButtons, false, 25.0f, 0.0f, -5.0f, 30.0f, 30.0f);
GLuint btmPlate;
int mouseButton = -1;
bool bMousePressed = false;

bool g_bPause = true;
bool g_bOneStep = false;
bool g_bWireframe = false;

std::string g_sWindowTitle;
std::string g_sWindowTitleInfo;
int g_Width;
int g_Height;
int g_CurFrame;

CWorldSimulation g_WorldSim;
double g_dt = 0.0166666667; // 1 / 60 second
GLuint g_glLtAxis;

static void DrawTextGlut(const char* str, float x, float y, float z);
static void DrawTextGlut(const char* str, float x, float y);

void InitSimulation()
{
	g_WorldSim.m_bGPU = false;
	g_WorldSim.Create();
	g_CurFrame = 0;
}

GLuint GenerateAxis(float fAxisLength)
{
	GLuint list = glGenLists(1);

	GLfloat axisL = fAxisLength;
	GLfloat axis_matA[] = {0.8f, 0, 0, 0.6f };
	GLfloat axis_matB[] = {0.0, 0.0, 0.8f, 0.6f };

	glNewList(list, GL_COMPILE);	
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, axis_matA);
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glLineWidth(4);
	//glLineStipple(1, 0x0C0F);
	//glEnable(GL_LINE_STIPPLE);

	GLUquadricObj* quad = gluNewQuadric();

	glBegin(GL_LINES);

	//X Axis
	glPushMatrix();
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(axisL, 0.0, 0.0);
	glTranslatef(axisL, 0.0f, 0.0f);
	//gluCylinder(quad, 1.0, 0.0, 2.0, 5, 5);	
	glPopMatrix();

	//Y Axis
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, axisL, 0.0);	

	//Z Axis
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, axisL);	

	glEnd();

	glDisable(GL_BLEND);	

	glColor3f(0.0, 0.0, 0.0);
	DrawTextGlut("X", axisL + 0.3, 0.0, 0.0);
	DrawTextGlut("Y", 0.0, axisL + 0.3, 0.0);
	DrawTextGlut("Z", 0.0, 0.0, axisL + 0.3);

	glEnable(GL_LIGHTING);

	glLineWidth(1);
	glEndList();

	return list;
}


void InitGL()
{
	// Setting Camera Moving Sensitivity..
	hCamera.SetMouseSensitivity(0.1f, 0.02f, 0.1f);

	// Generating bottom plate...
	GLfloat gray[4] = {0.5f, 0.5f, 0.5f, 1.0f};
	GLfloat black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	btmPlate = hCamera.makeBottomPlate(gray, black, 80.0f, 80.0f, 10.0f, 0.0f);	

	// Generate an axis
	g_glLtAxis = GenerateAxis(5.0f);
}

void SetWindowTitle()
{
	if ( g_WorldSim.m_pNarrowPhase )
	{
		CNarrowPhaseCollisionDetection::CollisionAlgorithmType colAlgoType = g_WorldSim.m_pNarrowPhase->GetConvexCollisionAlgorithmType();

		if ( colAlgoType == CNarrowPhaseCollisionDetection::BIM )
		{
			g_sWindowTitleInfo = "Brute-force Iterative method";
		}
		else if ( colAlgoType == CNarrowPhaseCollisionDetection::CHF )
		{
			g_sWindowTitleInfo = "Convex Height Field method";
		}
		else if ( colAlgoType == CNarrowPhaseCollisionDetection::GJK_EPA )
		{
			g_sWindowTitleInfo = "GJK/EPA method";
		}
	}
}

void DrawTextGlut(const char* str, float x, float y) 
{
	glRasterPos2f(x, y);
	glColor3d(0.0, 0.0, 0.0);

	for (int i = 0; str[i] != '\0'; i++) 
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, str[i]);

}

void DrawTextGlut(const char* str, float x, float y, float z) 
{
	glRasterPos3f(x, y, z);
	glColor3d(0.0, 0.0, 0.0);

	for (int i = 0; str[i] != '\0'; i++) 
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, str[i]);

}

void OnRender() 
{
	SetWindowTitle();

	std::string sTitle = g_sWindowTitle;
	glutSetWindowTitle(sTitle.c_str());	

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

	glPushMatrix();
	glCallList(g_glLtAxis);
	glPopMatrix();

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

	if ( !g_bPause )
	{
		if ( g_bOneStep )
		{
			g_bOneStep = false;
			g_bPause = true;
		}

		g_WorldSim.Update(g_dt);
		g_CurFrame++;
	}
	
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	g_WorldSim.Render(g_bWireframe);	

	//------------
	// Draw texts
	//------------
	/* We are going to do some 2-D orthographic drawing. */
    glMatrixMode(GL_PROJECTION);
	glPushMatrix();
    glLoadIdentity();

	int w = g_Width;
    int h = g_Height;

    GLdouble size = (GLdouble)((w >= h) ? w : h) / 2.0;
    GLdouble aspect;

    if (w <= h) {
        aspect = (GLdouble)h/(GLdouble)w;
        glOrtho(-size, size, -size*aspect, size*aspect, -1000000.0, 1000000.0);
    }
    else {
        aspect = (GLdouble)w/(GLdouble)h;
        glOrtho(-size*aspect, size*aspect, -size, size, -1000000.0, 1000000.0);
    }

    /* Make the world and window coordinates coincide so that 1.0 in */
    /* model space equals one pixel in window space.                 */
    glScaled(aspect, aspect, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();	
	
	std::string sInfo = "Current Algorithm(press 'a' to change): ";
	sInfo.append(g_sWindowTitleInfo);

	int linePos = 0;
	
	DrawTextGlut(sInfo.c_str(), -g_Width/2 + 10, g_Height/2 - (linePos += 20));

	// frame
	char frame[10];
	itoa(g_CurFrame, frame, 10);
	sInfo = "Frame: ";
	sInfo.append(frame);
	DrawTextGlut(sInfo.c_str(), -g_Width/2 + 10, g_Height/2 - (linePos += 20));

	// batch
	char batch[5];
	itoa(g_WorldSim.m_RenderBatchIndex, batch, 10);
	sInfo = "Batch: ";
	sInfo.append(batch);
	DrawTextGlut(sInfo.c_str(), -g_Width/2 + 10, g_Height/2 - (linePos += 20));

	// toggle CPU/GPU
	if ( g_WorldSim.m_bGPU )
		sInfo = "GPU solver";
	else
		sInfo = "CPU solver";

	DrawTextGlut(sInfo.c_str(), -g_Width/2 + 10, g_Height/2 - (linePos += 20));

	// help for keys
	DrawTextGlut("===========================================", -g_Width/2 + 10, g_Height/2 - (linePos += 20));
	DrawTextGlut("space: advance one step", -g_Width/2 + 10, g_Height/2 - (linePos += 20));
	DrawTextGlut("'s': start or stop", -g_Width/2 + 10, g_Height/2 - (linePos += 20));
	DrawTextGlut("'c': reset", -g_Width/2 + 10, g_Height/2 - (linePos += 20));
	DrawTextGlut("'b' : next batch", -g_Width/2 + 10, g_Height/2 - (linePos += 20));
	DrawTextGlut("'g' : toggle CPU/GPU solver", -g_Width/2 + 10, g_Height/2 - (linePos += 20));

	glMatrixMode(GL_PROJECTION);	
	glPopMatrix();	

	glMatrixMode(GL_MODELVIEW);	
	glPopMatrix();
			
	glutSwapBuffers();
}

void OnIdle()
{	
	glutPostRedisplay();	
}

void OnReshape(int w, int h) 
{
	g_Width = w;
	g_Height = h;

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

	case 's':
	case 'S':
		g_bPause = !g_bPause;
		break;

	case 'c':
	case 'C':
		{
			bool bPausePrev = g_bPause;
			g_bPause = true;
			g_WorldSim.ClearAll();
			g_WorldSim.Create();
			g_bPause = bPausePrev;
		}
		break;
	case 'b':
	case 'B':
		{
			bool bPausePrev = g_bPause;
			g_bPause = true;
			g_WorldSim.m_RenderBatchIndex++;
			g_bPause = bPausePrev;
		}
		break;

	case 'g':
	case 'G':
		{
			bool bPausePrev = g_bPause;
			g_bPause = true;
			g_WorldSim.m_bGPU = !g_WorldSim.m_bGPU;
			g_WorldSim.ClearAll();
			g_WorldSim.Create();
			g_bPause = bPausePrev;
		}
		break;

	case 32: // space
		g_bPause = false;
		g_bOneStep = true;	
		break;

	case 'w': 
	case 'W': 
		g_bWireframe = !g_bWireframe;	
		break;

	case 'a':
	case 'A':
		{
			if ( g_WorldSim.m_pNarrowPhase )
			{
				bool bPausePrev = g_bPause;
				g_bPause = true;

				CNarrowPhaseCollisionDetection::CollisionAlgorithmType colAlgoType = g_WorldSim.m_pNarrowPhase->GetConvexCollisionAlgorithmType();

				if ( colAlgoType == CNarrowPhaseCollisionDetection::GJK_EPA )
				{
					colAlgoType = CNarrowPhaseCollisionDetection::BIM;					
				}
				else if ( colAlgoType == CNarrowPhaseCollisionDetection::BIM )
				{
					colAlgoType = CNarrowPhaseCollisionDetection::CHF;
				}
				else if ( colAlgoType == CNarrowPhaseCollisionDetection::CHF )
				{
					colAlgoType = CNarrowPhaseCollisionDetection::GJK_EPA;
				}

				g_WorldSim.m_pNarrowPhase->SetConvexCollisionAlgorithmType(colAlgoType);

				g_bPause = bPausePrev;
			}
		}

		break;
	}
}

void main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(width, height);

	g_sWindowTitle = "Convex collision Algorithms - Dongsoo Han";
	glutCreateWindow(g_sWindowTitle.c_str());

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

