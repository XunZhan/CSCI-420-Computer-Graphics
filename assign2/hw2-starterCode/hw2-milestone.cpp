/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster
  C++ starter code

  Student username: <type your USC username here>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "openGLHeader.h"
#include "imageIO.h"

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "glutHeader.h"
#include <OpenGL/gl3.h>
#include <iostream>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>

#if defined(WIN32) || defined(_WIN32)
#ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

int mousePos[2]; // x,y coordinate of the mouse position
int leftMouseButton = 0; // 1 if pressed, 0 if not
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = TRANSLATE;
// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework II";
unsigned int VBO, VAO;
OpenGLMatrix openGLMatrix;
BasicPipelineProgram pipelineProgram;
GLuint program;
GLint h_modelViewMatrix, h_projectionMatrix;
int vertexNum, indexNum, restartIndex;
GLuint pos_loc, color_loc;

int intervalNum = 0;
float s = 0.5;
int pointPerInterval = 51;

using namespace std;

// represents one control point along the spline 
struct Point 
{
  double x;
  double y;
  double z;
};

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline 
{
  int numControlPoints;
  Point * points;
};

// the spline array 
Spline * splines;
// total number of splines 
int numSplines;

int loadSplines(char * argv) 
{
  char * cName = (char *) malloc(128 * sizeof(char));
  FILE * fileList;
  FILE * fileSpline;
  int iType, i = 0, j, iLength;

  // load the track file 
  fileList = fopen(argv, "r");
  if (fileList == NULL) 
  {
    printf ("can't open file\n");
    exit(1);
  }
  
  // stores the number of splines in a global variable 
  fscanf(fileList, "%d", &numSplines);

  splines = (Spline*) malloc(numSplines * sizeof(Spline));

  // reads through the spline files 
  for (j = 0; j < numSplines; j++) 
  {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL) 
    {
      printf ("can't open file\n");
      exit(1);
    }

    // gets length for spline file
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    // allocate memory for all the points
    splines[j].points = (Point *)malloc(iLength * sizeof(Point));
    splines[j].numControlPoints = iLength;

    // saves the data to the struct
    while (fscanf(fileSpline, "%lf %lf %lf", 
	   &splines[j].points[i].x, 
	   &splines[j].points[i].y, 
	   &splines[j].points[i].z) != EOF) 
    {
      i++;
    }
  }

  free(cName);

  return 0;
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)
  
  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };
  
  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (rightMouseButton)
      {
        // control z translation via the right mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;
      
      // rotate the landscape
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
      }
      if (rightMouseButton)
      {
        // control z rotation via the right mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;
      
      // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (rightMouseButton)
      {
        // control z scaling via the right mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }
  
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed
  
  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
      break;
    
    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
      break;
    
    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
      break;
  }
  
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
      // glut cannot keep track of the CTRL key for Mac (which is Command on Mac), change the detection method
    case 't':
      controlState = TRANSLATE;
      break;
    
    case 's':
      controlState = SCALE;
      break;
    
    case 'r':
      controlState = ROTATE;
      break;
  }
  
}

void idleFunc()
{
  // do some stuff...
  
  // for example, here, you can save the screenshots to disk (to make the animation)
  
  // make the screen update
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);
  
  // setup perspective matrix...
  openGLMatrix.SetMatrixMode(OpenGLMatrix::Projection);
  openGLMatrix.LoadIdentity();
  openGLMatrix.Perspective(45.0, 1.0 * w / h, 0.01, 10.0);
  openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
}

void renderFunc()
{
  int start = 0;
  for(int i=0; i<numSplines; i++)
  {
    int pt = (splines[i].numControlPoints - 3)*pointPerInterval;
    glDrawArrays(GL_LINE_STRIP, start, pt);
    start += pt;
  }
  
}

void displayFunc()
{
  // render some stuff...
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  pipelineProgram.Bind();
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  
  glEnableVertexAttribArray(pos_loc);
  glVertexAttribPointer(pos_loc, 3, GL_FLOAT, GL_FALSE, 3* sizeof(float), (void*) 0);
  glEnableVertexAttribArray(color_loc);
  glVertexAttribPointer(color_loc, 4, GL_FLOAT, GL_FALSE, 4* sizeof(float), (void*) (3 * vertexNum * sizeof(float)));
  
  float m[16];
  openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
  openGLMatrix.LoadIdentity();
  openGLMatrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
  openGLMatrix.Rotate(landRotate[0], 1, 0, 0);
  openGLMatrix.Rotate(landRotate[1], 0, 1, 0);
  openGLMatrix.Rotate(landRotate[2], 0, 0, 1);
  openGLMatrix.Scale(landScale[0], landScale[1], landScale[2]);
  // openGLMatrix.LookAt(0.05,0.4,0.05,  0,0,0,  0,1,0);
  // openGLMatrix.LookAt(0.5, 0.5, 1.5, 0.5, 0.5, 0.5, 0, 1, 0);
  openGLMatrix.LookAt(0, 1.5, 1.5, 0, 0.3, 0, 0, 1, 0);
  openGLMatrix.GetMatrix(m);
  glUniformMatrix4fv(h_modelViewMatrix, 1, GL_FALSE, m);
  
  float p[16];
  openGLMatrix.SetMatrixMode(OpenGLMatrix::Projection);
  openGLMatrix.GetMatrix(p);
  glUniformMatrix4fv(h_projectionMatrix, 1, GL_FALSE, p);
  
  renderFunc();
  
  glBindVertexArray(0);
  glutSwapBuffers();
}


void initScene(int argc, char *argv[]) {
  // do additional initialization here...
  glEnable(GL_DEPTH_TEST);
  
  // load the splines from the provided filename
  loadSplines(argv[1]);
  
  printf("Loaded %d spline(s).\n", numSplines);
  for(int i=0; i<numSplines; i++)
  {
    printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);
    intervalNum = intervalNum + splines[i].numControlPoints - 3;
  }
  
  // do additional initialization here...
  // generate model vertices
  vertexNum = intervalNum * pointPerInterval;
  float pointPos[3 * vertexNum];
  float pointCol[4 * vertexNum];
  
  int vCount = 0;
  for (int i = 0; i<numSplines; i++)
  {
    for (int j = 1; j<splines[i].numControlPoints-2; j++)
    {
      glm::mat3x4 control =glm::mat3x4(
              splines[i].points[j-1].x, splines[i].points[j].x, splines[i].points[j+1].x, splines[i].points[j+2].x,
              splines[i].points[j-1].y, splines[i].points[j].y, splines[i].points[j+1].y, splines[i].points[j+2].y,
              splines[i].points[j-1].z, splines[i].points[j].z, splines[i].points[j+1].z, splines[i].points[j+2].z
              );
      glm::mat4 basis = glm::mat4(
               -s,   2*s, -s, 0,
              2-s,   s-3,  0, 1,
              s-2, 3-2*s,  s, 0,
                s,    -s,  0, 0
                );
      
      for (int k = 0; k<pointPerInterval; k++)
      {
        float u = k*(1.0/(pointPerInterval-1));
        glm::vec4 uu(u*u*u, u*u, u, 1);
        glm::vec3 point =  glm::transpose(control) * glm::transpose(basis) * uu ;
        
        pointPos[vCount*3+0] = point.x;
        pointPos[vCount*3+1] = point.y;
        pointPos[vCount*3+2] = point.z;
        
        pointCol[vCount*4+0] = 1.0;
        pointCol[vCount*4+1] = 1.0;
        pointCol[vCount*4+2] = 1.0;
        pointCol[vCount*4+3] = 1.0;
        vCount++;
      }
    }
  }
  
  // init the program
  pipelineProgram.Init("../openGLHelper-starterCode");
  pipelineProgram.Bind();
  program = pipelineProgram.GetProgramHandle();
  h_modelViewMatrix = glGetUniformLocation(program, "modelViewMatrix");
  h_projectionMatrix = glGetUniformLocation(program, "projectionMatrix");
  pos_loc = glGetAttribLocation(program, "position");
  color_loc = glGetAttribLocation(program, "color");
  
  // make the VAO
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
  // make the VBO
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, 3 * vertexNum * sizeof(float) + 4 * vertexNum * sizeof(float), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * vertexNum * sizeof(float), pointPos);
  glBufferSubData(GL_ARRAY_BUFFER, 3 * vertexNum * sizeof(float), 4 * vertexNum * sizeof(float), pointCol);
}

int initTexture(const char * imageFilename, GLuint textureHandle)
{
  // read the texture image
  ImageIO img;
  ImageIO::fileFormatType imgFormat;
  ImageIO::errorType err = img.load(imageFilename, &imgFormat);

  if (err != ImageIO::OK) 
  {
    printf("Loading texture from %s failed.\n", imageFilename);
    return -1;
  }

  // check that the number of bytes is a multiple of 4
  if (img.getWidth() * img.getBytesPerPixel() % 4) 
  {
    printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
    return -1;
  }

  // allocate space for an array of pixels
  int width = img.getWidth();
  int height = img.getHeight();
  unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

  // fill the pixelsRGBA array with the image pixels
  memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
  for (int h = 0; h < height; h++)
    for (int w = 0; w < width; w++) 
    {
      // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
      pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
      pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
      pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
      pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

      // set the RGBA channels, based on the loaded image
      int numChannels = img.getBytesPerPixel();
      for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
        pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
    }

  // bind the texture
  glBindTexture(GL_TEXTURE_2D, textureHandle);

  // initialize the texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

  // generate the mipmaps for this texture
  glGenerateMipmap(GL_TEXTURE_2D);

  // set the texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // query support for anisotropic texture filtering
  GLfloat fLargest;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
  printf("Max available anisotropic samples: %f\n", fLargest);
  // set anisotropic texture filtering
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

  // query for any errors
  GLenum errCode = glGetError();
  if (errCode != 0) 
  {
    printf("Texture initialization error. Error code: %d.\n", errCode);
    return -1;
  }
  
  // de-allocate the pixel array -- it is no longer needed
  delete [] pixelsRGBA;

  return 0;
}

// Note: You should combine this file
// with the solution of homework 1.

// Note for Windows/MS Visual Studio:
// You should set argv[1] to track.txt.
// To do this, on the "Solution Explorer",
// right click your project, choose "Properties",
// go to "Configuration Properties", click "Debug",
// then type your track file name for the "Command Arguments".
// You can also repeat this process for the "Release" configuration.

int main (int argc, char ** argv)
{
  if (argc<2)
  {  
    printf ("usage: %s <trackfile>\n", argv[0]);
    exit(0);
  }
  
  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);
  
  cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
  glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif
  
  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);
  glutCreateWindow(windowTitle);
  
  // tells glut to use a particular display function to redraw
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);
  
  // init glew
#ifdef __APPLE__
  // nothing is needed on Apple
#else
  // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
#endif
  
  // do initialization
  initScene(argc, argv);
  
  // sink forever into the glut loop
  glutMainLoop();
  
  
  return 0;
}

