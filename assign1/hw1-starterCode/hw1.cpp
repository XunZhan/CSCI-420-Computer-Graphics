/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields
  C++ starter code

  Student username: <type your USC username here>
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
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

using namespace std;

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

typedef enum { POINT, LINE, FILLED } RENDER_MODE;
RENDER_MODE renderMode = LINE;

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

ImageIO * heightmapImage;
unsigned int VBO, EBO, VAO;
OpenGLMatrix openGLMatrix;
BasicPipelineProgram pipelineProgram;
GLuint program;
GLint h_modelViewMatrix, h_projectionMatrix;
int vertexNum, indexNum, restartIndex;
GLuint pos_loc, color_loc;

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

void renderFunc()
{
  switch (renderMode)
  {
    case POINT:
      glDrawElements(GL_POINTS, indexNum, GL_UNSIGNED_INT, 0);
      break;
    case LINE:
      glDrawElements(GL_TRIANGLE_STRIP, indexNum, GL_UNSIGNED_INT, 0);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      break;
    case FILLED:
      glDrawElements(GL_TRIANGLE_STRIP, indexNum, GL_UNSIGNED_INT, 0);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      break;
  }
}

void displayFunc()
{
  // render some stuff...
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_PRIMITIVE_RESTART);
  glPrimitiveRestartIndex(restartIndex);
  
  pipelineProgram.Bind();
  glBindVertexArray(VAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
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
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // take a screenshot
      saveScreenshot("screenshot.jpg");
    break;
    
    // change render mode
    case 'p':
      renderMode = POINT;
      break;
      
    case 'l':
      renderMode = LINE;
      break;
      
    case 'f':
      renderMode = FILLED;
      break;
    
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

void initScene(int argc, char *argv[]) {
  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK) {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }
  glEnable(GL_DEPTH_TEST);
  
  // do additional initialization here...
  // generate model vertices
  int imageHeight = heightmapImage->getHeight();
  int imageWidth = heightmapImage->getWidth();
  vertexNum = imageHeight * imageWidth;
  indexNum = (imageHeight - 1) * (imageWidth*2 + 1);
  float pointPos[3 * vertexNum];
  float pointCol[4 * vertexNum];
  int pointIdx[indexNum];
  
  unsigned char *pixels = heightmapImage->getPixels();
  float maxHeight = *max_element(pixels, pixels + vertexNum);
  // float minHeight = *min_element(pixels, pixels+verticesNum);
  
  for (int i = 0; i < imageWidth; i++) {
    for (int j = 0; j < imageHeight; j++) {
      pointPos[(j * imageWidth + i) * 3] = (float) i / imageWidth - 0.5;
      pointPos[(j * imageWidth + i) * 3 + 1] = heightmapImage->getPixel(i, j, 0) / maxHeight;
      pointPos[(j * imageWidth + i) * 3 + 2] = (float) j / imageHeight - 0.5;
      
      pointCol[(j * imageWidth + i) * 4] = pointPos[(j * imageWidth + i) * 3] + 0.5;
      pointCol[(j * imageWidth + i) * 4 + 1] = pointPos[(j * imageWidth + i) * 3 + 1];
      pointCol[(j * imageWidth + i) * 4 + 2] = pointPos[(j * imageWidth + i) * 3 + 2] + 0.5;
      pointCol[(j * imageWidth + i) * 4 + 3] = 1;
    }
  }
  
  // init facets index for triangle
  restartIndex = vertexNum;
  bool isDown = true;
  for (int row = 0; row < imageHeight - 1; row++) {
    pointIdx[row * (imageWidth*2 + 1)] = (row) * imageWidth;
    for (int col = 1; col < imageWidth*2; col++)
    {
      if (isDown)
        pointIdx[row * (imageWidth*2 + 1) + col] = pointIdx[row * (imageWidth*2 + 1) + col - 1] + imageWidth;
      else
        pointIdx[row * (imageWidth*2 + 1) + col] = pointIdx[row * (imageWidth*2 + 1) + col - 1] - (imageWidth - 1);
      
      isDown = !isDown;
    }
    isDown = true;
    pointIdx[row * (imageWidth*2 + 1) + imageWidth*2] =vertexNum;
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
  
  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,  (imageHeight - 1) * (imageWidth*2 + 1) * sizeof(int), pointIdx, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
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

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

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
}


