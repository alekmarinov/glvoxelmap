
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <gl/glu.h>
#include "resource.h"
#include "3DMath.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// The title bar text
HWND g_hWnd;                                    // Main window handle
HDC  g_hDC;                                     // Window device context
HGLRC g_hglRC;                                  // GL Render Context

// Forward declarations of functions included in this code module:

// Windows related
ATOM				RegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
void				DoneInstance();
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

// GL related
void                InitOpenGL();
void                InitScene();
void                DoneScene();
void                ChangeSize(int, int);
void                RenderScene();
void                DoneOpenGL();
void                createmaps();
void                createfractalmap(int x1, int y1, int x2, int y2);
void                cleanup();
void                smoothmap();
char                newheight(int mc, int n, int dvd);

// A couple of miscelanous constants
#define  NUMSMOOTH   3     // smooth the map count
#define  RANDOM(a)   (rand() / (RAND_MAX / (a))) // a simple random function
#define  MAPSIZE     256
#define  INDEX(x,y)  ((((y) % MAPSIZE) * MAPSIZE) + (x) % MAPSIZE) // index calculation

// Globals

char *heightmap;          // height feild for the voxels
int mouseX, mouseY;
int width, height;
bool bDragStart = 0;
int dragDX = 0, dragX = 0;
int dragDY = 0, dragY = 0;
float scale = 2;//0.01;
int mouseEventL = 0;
int mouseEventR = 0;

int phase = 0;

struct CAMERA
{
    CVector3 eye;
    CVector3 view;
    CVector3 up;
} camera;


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_GLDEMO, szWindowClass, MAX_LOADSTRING);
	RegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_GLDEMO);

    InitOpenGL();
    InitScene();

    createmaps();

	while(1)											// Do our infinate loop
	{													// Check if there was a message
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
        { 
            if (msg.message == WM_QUIT) break;
		    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		    {
			    TranslateMessage(&msg);
			    DispatchMessage(&msg);
		    }
        }
		else											// if there wasn't a message
		{ 
			RenderScene();								
        } 
	}

    DoneScene();
    DoneOpenGL();
    DoneInstance();

    PostQuitMessage(0);
	return msg.wParam;
}


ATOM RegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_GLDEMO);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCSTR)IDC_GLDEMO;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   g_hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!g_hWnd)
   {
      return FALSE;
   }

   ShowWindow(g_hWnd, nCmdShow);
   UpdateWindow(g_hWnd);
   return TRUE;
}

void DoneInstance()
{
    UnregisterClass(szWindowClass, hInst);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message) 
	{
		case WM_SIZE:
            ChangeSize(LOWORD(lParam), HIWORD(lParam));
            break;

		case WM_LBUTTONDOWN:
                mouseEventL = 1;
                bDragStart = true;
                dragX = LOWORD(lParam);
                dragY = HIWORD(lParam);
            break;

		case WM_RBUTTONDOWN:
                mouseEventR = 1;
            break;

		case WM_LBUTTONUP:
                bDragStart = false;
                mouseEventL = 0;
            break;

		case WM_RBUTTONUP:
                bDragStart = false;
                mouseEventR = 0;
            break;

		case WM_MOUSEMOVE:
            {
                mouseX = LOWORD(lParam);  // horizontal position of cursor 
                mouseY = HIWORD(lParam);  // vertical position of cursor 

                dragDX = dragX - mouseX;
                dragDY = dragY - mouseY;
            }
            break;

        case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDM_ABOUT:
				   DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
				   break;
				case IDM_EXIT:
				   DestroyWindow(hWnd);
				   break;
				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

// Mesage handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}


void InitOpenGL()
{
    g_hDC = GetDC(g_hWnd);

    int iPixelFormat;
    static PIXELFORMATDESCRIPTOR pfd = 
    { 
        sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd 
        1,                     // version number 
        PFD_DRAW_TO_WINDOW |   // support window 
        PFD_SUPPORT_OPENGL |   // support OpenGL 
        PFD_DOUBLEBUFFER,      // double buffered 
        PFD_TYPE_RGBA,         // RGBA type 
        24,                    // 24-bit color depth 
        0, 0, 0, 0, 0, 0,      // color bits ignored 
        0,                     // no alpha buffer 
        0,                     // shift bit ignored 
        0,                     // no accumulation buffer 
        0, 0, 0, 0,            // accum bits ignored 
        32,                    // 32-bit z-buffer 
        0,                     // no stencil buffer 
        0,                     // no auxiliary buffer 
        PFD_MAIN_PLANE,        // main layer 
        0,                     // reserved 
        0, 0, 0                // layer masks ignored 
    }; 
    iPixelFormat = ChoosePixelFormat(g_hDC, &pfd); 
    SetPixelFormat(g_hDC, iPixelFormat, &pfd); 
 
    g_hglRC = wglCreateContext(g_hDC);
    wglMakeCurrent (g_hDC, g_hglRC);

    RECT rRect;

	GetClientRect(g_hWnd, &rRect);					

    width = rRect.right;
    height = rRect.bottom;

	glViewport(0,0,width,height);						
														
	glMatrixMode(GL_PROJECTION);						
	glLoadIdentity();									

														// Calculate The Aspect Ratio Of The Window
														// The parameters are:
														// (view angle, aspect ration of the width to the height, 
														//  The closest distance to the camera before it clips, 
				  // FOV		// Ratio				//  The farthest distance before it stops drawing)
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height, 0.5f , 160);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
    glColorMask(GL_TRUE, GL_TRUE, GL_FALSE, GL_FALSE);

    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void DoneOpenGL()
{
    if (g_hglRC)
    {
        wglMakeCurrent (NULL, NULL) ; 
        wglDeleteContext (g_hglRC); 
    }

    if (g_hDC)
    {
        ReleaseDC(g_hWnd, g_hDC);
    }
}

void InitScene()
{

    camera.eye = CVector3(-4, -4, 1);
    camera.view = CVector3(0.1, 0.6, 0);
    camera.up = CVector3(0, 0, 1);
}

void DoneScene()
{
    cleanup();
}

float f = 0;
float rotZ = 0;
float rotDZ = 0.1;
float rotDZAcc = 0.1;
int counter = 0;
int zLimAngle = 15;
float rotXY = 0;
float rotDXY = 10;
float distance = -5;

void RenderScene()
{
    camera.eye.x = distance;
    camera.eye.y = distance;
    camera.eye.z = 1;


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	glLoadIdentity();									
    
    gluLookAt(
            camera.eye.x, camera.eye.y, camera.eye.z, 
            camera.view.x, camera.view.y, camera.view.z, 
            camera.up.x, camera.up.y, camera.up.z);

    //glRotatef(-90 + rotXY, 1, 0, 0);
    //glRotatef(-45 + rotXY, 0, 1, 0);
    glRotatef(-90 + -mouseX, 1, 0, 0);
    glRotatef(-45 + mouseY, 0, 1, 0);

	glColor3ub(128, 128, 128);							

    if (mouseEventL)
    {
        rotXY += rotDXY;
        if (rotXY > 180 || rotXY < -180)
            rotDXY -= rotDXY;

        distance -= 0.1;
    }

    if (mouseEventR)
    {
        rotXY -= rotDXY;
        if (rotXY > 180 || rotXY < -180)
            rotDXY -= rotDXY;
        distance += 0.1;
    }


    glPushMatrix();

       float div = (double)MAPSIZE / 16;
        //glTranslatef( - ((double)MAPSIZE / div / 2), - ((double)MAPSIZE / div / 2), -0.3);
        glRotatef(rotZ += rotDZ, 0, 0, 1); 
        //if (rotZ > zLimAngle || rotZ < -zLimAngle) rotDZ = -rotDZ;

        glBegin (GL_QUADS );								    

            for (int x = 0; x < MAPSIZE; x++)
               for (int y = 0; y < MAPSIZE; y++)
               {
                   float c = (double)heightmap[INDEX(x,y)] / 256;
                   if (c < 5) c*=4;
                   glColor3f(c, c, c);
                   glVertex3f((double)x / div - (MAPSIZE / div / 2), (double)y / div - (MAPSIZE / div / 2), scale*(double)heightmap[INDEX(x,y)] / 256);
                   glVertex3f((double)(x+1) / div - (MAPSIZE / div / 2), (double)y / div - (MAPSIZE / div / 2), scale*(double)heightmap[INDEX(x+1,y)] / 256);
                   glVertex3f((double)x / div - (MAPSIZE / div / 2), (double)(y+1) / div - (MAPSIZE / div / 2), scale*(double)heightmap[INDEX(x,y+1)] / 256);
                   glVertex3f((double)(x+1) / div - (MAPSIZE / div / 2), (double)(y+1) / div - (MAPSIZE / div / 2), scale*(double)heightmap[INDEX(x+1,y+1)] / 256);
               }


	    glEnd();											
    glPopMatrix();

    glBegin(GL_LINES);
        glColor3f(1, 0, 0);     // x in red
        glVertex3f(0, 0, 0);
        glVertex3f(1, 0, 0);

        glColor3f(0, 1, 0);     // y in green
        glVertex3f(0, 0, 0);
        glVertex3f(0, 1, 0);

        glColor3f(0, 0, 0);     // z in black
        glVertex3f(0, 0, 0);
        glVertex3f(0, 0, 1);

/*
        glColor3f(1, 1, 1);
        glVertex3f(0, 0, 0);
        glVertex3f(camera.view.x, camera.view.y, camera.view.z);
*/
    glEnd();

	SwapBuffers(g_hDC);									
}

void createmaps()
{
    // allocate necessary memory
    heightmap = (char *)malloc(MAPSIZE*MAPSIZE);

    // clear all memory
    memset(heightmap, 0, MAPSIZE*MAPSIZE);

    heightmap[0] = 64;   // initialize starting point on map
    createfractalmap(0, 0, MAPSIZE, MAPSIZE);

    for (int i = 0; i < NUMSMOOTH; i++)
      smoothmap();
}

void cleanup()
// free allocated memory
{
    free(heightmap);
}

void createfractalmap(int x1, int y1, int x2, int y2)
// recursive fractal terrain builder
{
    int p1, p2, p3, p4;
    int xn, yn, dxy;

    if (((x2-x1) < 2) && ((y2-y1) < 2))  // make sure their is something to do
    {
      return;
    }

    p1 = heightmap[INDEX(x1,y1)];
    p2 = heightmap[INDEX(x1,y2)];
    p3 = heightmap[INDEX(x2,y1)];
    p4 = heightmap[INDEX(x2,y2)];

    xn = (x2+x1) >> 1;
    yn = (y2+y1) >> 1;
    dxy = 5*(x2 - x1 + y2 - y1) / 3;

    if (heightmap[INDEX(xn,y1)] == 0)
      heightmap[INDEX(xn,y1)] = newheight(p1+p3, dxy, 2);
    if (heightmap[INDEX(x1,yn)] == 0)
      heightmap[INDEX(x1,yn)] = newheight(p2+p4, dxy, 2);
    if (heightmap[INDEX(x2,yn)] == 0)
      heightmap[INDEX(x2,yn)] = newheight(p3+p4, dxy, 2);
    if (heightmap[INDEX(xn,y2)] == 0)
      heightmap[INDEX(xn,y2)] = newheight(p1+p2, dxy, 2);
    heightmap[INDEX(xn,yn)] = newheight(p1+p2+p3+p4, dxy, 4);

    createfractalmap(x1, y1, xn, yn);
    createfractalmap(xn, y1, x2, yn);
    createfractalmap(x1, yn, xn, y2);
    createfractalmap(xn, yn, x2, y2);
}

char newheight(int mc, int n, int dvd)
{
    int loc;

    loc = (mc + n - RANDOM(n << 1)) / dvd - 1;
    if (loc > 255)
      loc = 255;
    if (loc < 10)
      loc = 10;
    return(loc);
}

void smoothmap()
// smooths the map. Gives better appearence
{
    int x,y;

    for (x = 1; x < MAPSIZE; x++)
      for (y = 1; y < MAPSIZE; y++)
        heightmap[INDEX(x,y)] = (heightmap[INDEX(x-1,y-1)] +
                                 heightmap[INDEX(x-1,y+1)] +
                                 heightmap[INDEX(x+1,y-1)] +
                                 heightmap[INDEX(x+1,y+1)] +
                                 heightmap[INDEX(x,  y-1)] +
                                 heightmap[INDEX(x,  y+1)] +
                                 heightmap[INDEX(x-1,y)] +
                                 heightmap[INDEX(x+1,y)]) >> 3;
}

void ChangeSize(int w, int h)
{
    glViewport(0, 0, width = w, height = h);
}
