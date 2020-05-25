/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster
  C++ starter code

  Student username: <type your USC username here>
*/
#include <GLUT/glut.h>
#include <OpenGL/gl3.h>
#include <iostream>
#include <cstring>
#include <iomanip>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imageIO.h"
#include <vector>
#include <random>
#include <chrono>

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "Comet.h"

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

#define PI 3.1415926

int mousePos[2]; // x,y coordinate of the mouse position
int leftMouseButton = 0; // 1 if pressed, 0 if not
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum {DAY, NIGHT} ENV_STATE;
ENV_STATE envState = DAY;
typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = TRANSLATE;
bool stopWorld = false;
// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework II";
unsigned int VBO, VAO, leftVBO, rightVBO, middleVBO, CSVAO, CSVAO2;
unsigned int skyVBO, skyVAO, sphereVAO, sphereVBO, sphereEBO, cometVAO, cometVBO;
OpenGLMatrix openGLMatrix;
BasicPipelineProgram pipelineProgram, skyProgram, sphereProgram, cometProgram;
GLuint trackPH, skyPH, spherePH, cometPH;
GLint h_modelViewMatrix, h_projectionMatrix;
GLint sky_modelViewMatrix, sky_projectionMatrix;
GLint sphere_modelViewMatrix, sphere_projectionMatrix;
GLint comet_modelViewMatrix, comet_projectionMatrix;
GLuint t_posLoc, t_texLoc, t_normLoc, t_cameraPos, t_lightPos, t_lightColor, t_objectColor;
GLuint s_posLoc, c_posLoc, c_posColor;
GLuint day_texture, night_texture, track_texture, moon_texture;
GLuint h_posLoc, h_normLoc;
glm::vec3 cameraPos;
glm::vec3 lightColor_day = glm::vec3(1.0, 0.9, 0.8);
glm::vec3 lightColor_night = glm::vec3(0.8, 0.9, 1.0);
glm::vec3 lightPos = glm::vec3(0.5, 0.7, 0.9);
glm::vec3 objectColor = glm::vec3(1.0, 1.0, 1.0);

float waitBeforeMove = 80; //ms
int curIndex = 0;          // current moving to this index
int intervalNum = 0;
float s = 0.5;
int pointPerInterval = 51;

chrono::steady_clock::time_point begin_time;
chrono::steady_clock::time_point currt_time;

vector<glm::vec3> pointPos;
vector<glm::vec3> pointTan;
vector<glm::vec3> pointNorm;
vector<glm::vec3> pointBino;
vector<glm::vec3> leftTrack;
vector<glm::vec3> rightTrack;
vector<glm::vec3> middleTrack;
vector<glm::vec3> leftTrackNormal;
vector<glm::vec3> rightTrackNormal;
vector<glm::vec3> middleTrackNormal;
vector<glm::vec3> spherePos;
vector<glm::vec3> sphereNorm;
vector<int> sphereIndex;
vector<Comet> comets;
vector<glm::vec3> cometPos;
vector<int> cometColor;

// the spline array
Spline * splines;
// total number of splines
int numSplines;

int loadCubeMap(vector<char*> faces);
void MoveCamera(int pIdx);
int initTexture(const char * imageFilename, GLuint textureHandle);

float RandomValue(float low, float high)
{
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_real_distribution<float> dist(low, high);
  
  return dist(rng);
}

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

    double curMax = -DBL_MAX;
    // saves the data to the struct
    while (fscanf(fileSpline, "%lf %lf %lf",
	   &splines[j].points[i].x,
	   &splines[j].points[i].y,
	   &splines[j].points[i].z) != EOF)
    {
      double m = max( max(splines[j].points[i].x, splines[j].points[i].y), splines[j].points[i].z);
      curMax = max(curMax,m);
      i++;
    }
  
    // Normalize
    if (curMax > 1)
    {
      for (int k = 0; k<splines[j].numControlPoints; k++)
      {
        splines[j].points[k].x /= curMax;
        splines[j].points[k].y /= curMax;
        splines[j].points[k].z /= curMax;
      }
    }
    
    //rotate
    for (int k = 0; k<splines[j].numControlPoints;k++)
    {
      glm::vec3 v(splines[j].points[k].x, splines[j].points[k].y, splines[j].points[k].z);
      glm::mat4 mr = glm::rotate(glm::radians(-120.0f), glm::vec3(1.0, 1.0, 1.0));
      glm::mat4 mr2 = glm::rotate(glm::radians(90.0f), glm::vec3(0.0, 1.0, 0.0));
      glm::mat4 mt = glm::translate( glm::vec3(-0.7, -0.7, -0.7));
      glm::mat4 mt2 = glm::translate( glm::vec3(0.0, 0.2, 0.1));
      glm::vec4 new_pos = mt2 * mr2 * mr * mt * glm::vec4(v,1.0);
      splines[j].points[k].x = new_pos.x;
      splines[j].points[k].y = new_pos.y;
      splines[j].points[k].z = new_pos.z;
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
        landTranslate[0] += mousePosDelta[0] * 0.005f;
        landTranslate[1] -= mousePosDelta[1] * 0.005f;
      }
      if (rightMouseButton)
      {
        // control z translation via the right mouse button
        landTranslate[2] += mousePosDelta[1] * 0.005f;
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
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.005f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.005f;
      }
      if (rightMouseButton)
      {
        // control z scaling via the right mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.005f;
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
      
    case 'd':
      envState = DAY;
      break;
      
    case 'n':
      envState = NIGHT;
      break;
      
    case 'p':
      stopWorld = !stopWorld;
    break;
  }
  
}

void timerFunc(int t)
{
  if (curIndex == pointPos.size()-1)
    curIndex = 0;
  if (stopWorld)
  {
    glutTimerFunc(waitBeforeMove, timerFunc, curIndex);
  } else{
    glutTimerFunc(waitBeforeMove, timerFunc, ++curIndex);
  }
  glutPostRedisplay();
}

void idleFunc()
{
  // do some stuff...
  
  // for example, here, you can save the screenshots to disk (to make the animation)
  
  // make the screen update
//  for (int i = 0; i<pointPos.size(); i++)
//  {
//    curIndex = i;
//    MoveCamera();
//  }

  curIndex = 1;
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

void renderSpline()
{
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  
  int start = 0;
  for(int i=0; i<numSplines; i++)
  {
    int pt = (splines[i].numControlPoints - 3)*pointPerInterval;
    glDrawArrays(GL_LINE_STRIP, start, pt);
    start += pt;
  }
  glBindVertexArray(0);
}

void renderCrossSection()
{
//  glEnable(GL_TEXTURE_2D);
//  glBindTexture(GL_TEXTURE_2D, track_texture);
  glEnable(GL_TEXTURE_CUBE_MAP);
  if (envState == DAY)
    glBindTexture(GL_TEXTURE_CUBE_MAP, day_texture);
  else
    glBindTexture(GL_TEXTURE_CUBE_MAP, night_texture);
  
  glBindVertexArray(CSVAO);
  //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, CSEBO);
  
  glBindBuffer(GL_ARRAY_BUFFER, leftVBO);
  glEnableVertexAttribArray(t_posLoc);
  glVertexAttribPointer(t_posLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
//  glEnableVertexAttribArray(t_texLoc);
//  glVertexAttribPointer(t_texLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)(leftTrack.size() * sizeof(glm::vec3)));
  glEnableVertexAttribArray(t_normLoc);
  glVertexAttribPointer(t_normLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)(leftTrack.size() * sizeof(glm::vec3)));
  //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  //glDrawElements(GL_TRIANGLES, CSIndex.size(), GL_UNSIGNED_INT, 0);
  //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDrawArrays(GL_TRIANGLES, 0, leftTrack.size());
  
  
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  glBindBuffer(GL_ARRAY_BUFFER, rightVBO);
  glEnableVertexAttribArray(t_posLoc);
  glVertexAttribPointer(t_posLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
//  glEnableVertexAttribArray(t_texLoc);
//  glVertexAttribPointer(t_texLoc, 2, GL_FLOAT, GL_FALSE, 0,  (void*)(rightTrack.size() * sizeof(glm::vec3)));
  glEnableVertexAttribArray(t_normLoc);
  glVertexAttribPointer(t_normLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)(rightTrack.size() * sizeof(glm::vec3)));
  //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  //glDrawElements(GL_TRIANGLES, CSIndex.size(), GL_UNSIGNED_INT, 0);
  glDrawArrays(GL_TRIANGLES, 0, rightTrack.size());
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  //glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  
  glBindVertexArray(CSVAO2);
  glBindBuffer(GL_ARRAY_BUFFER, middleVBO);
  glEnableVertexAttribArray(t_posLoc);
  glVertexAttribPointer(t_posLoc, 3, GL_FLOAT, GL_FALSE, 0 ,(void*)0);
  glEnableVertexAttribArray(t_normLoc);
  glVertexAttribPointer(t_normLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)(middleTrack.size() * sizeof(glm::vec3)));
  
  glDrawArrays(GL_TRIANGLES, 0, middleTrack.size());
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
    
    // glDisable(GL_TEXTURE_2D);
  glDisable(GL_TEXTURE_CUBE_MAP);
}

void renderSky()
{
  glEnable(GL_TEXTURE_CUBE_MAP);
  if (envState == DAY)
    glBindTexture(GL_TEXTURE_CUBE_MAP, day_texture);
  else
    glBindTexture(GL_TEXTURE_CUBE_MAP, night_texture);
  glBindVertexArray(skyVAO);
  glBindBuffer(GL_ARRAY_BUFFER,skyVBO);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glDisable(GL_TEXTURE_CUBE_MAP);
}

void renderSphere()
{
  glEnable(GL_TEXTURE_CUBE_MAP);
  glBindTexture(GL_TEXTURE_CUBE_MAP, moon_texture);
  glBindVertexArray(sphereVAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
  glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
  
  glEnableVertexAttribArray(h_posLoc);
  glVertexAttribPointer(h_posLoc, 3, GL_FLOAT,GL_FALSE, 0, (void*) 0);
  glEnableVertexAttribArray(h_normLoc);
  glVertexAttribPointer(h_normLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)(spherePos.size() * sizeof(glm::vec3)));
  //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDrawElements(GL_TRIANGLES,  sphereIndex.size(), GL_UNSIGNED_INT, (void*) 0);
  
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
  glBindVertexArray(0);
  glDisable(GL_TEXTURE_CUBE_MAP);
}

void UpdateComet()
{
  currt_time = chrono::steady_clock::now();
  for (int i = 0; i<comets.size(); i++)
  {
    chrono::duration<float, std::milli> d = currt_time - begin_time;
    float duration = d.count(); // ms
    float t = duration * comets[i].speed;

    while (t > comets[i].tmax)
      t = t - comets[i].tmax;

    cometPos[i*2] = comets[i].start + t * comets[i].direction;
    cometPos[i*2+1] = cometPos[i*2] - comets[i].len * comets[i].direction;
  }
}

void renderComet()
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE);
  
  UpdateComet();
  glBindVertexArray(cometVAO);
  glBindBuffer(GL_ARRAY_BUFFER, cometVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, cometPos.size() * sizeof(glm::vec3), cometPos.data());
  glBufferSubData(GL_ARRAY_BUFFER, cometPos.size() * sizeof(glm::vec3), cometColor.size() * sizeof(int), cometColor.data());
  
  glEnableVertexAttribArray(c_posLoc);
  glVertexAttribPointer(c_posLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
  glEnableVertexAttribArray(c_posColor);
  glVertexAttribPointer(c_posColor, 1, GL_INT, GL_FALSE, 0, (void*)(comets.size() * 2 * sizeof(glm::vec3)));
  glDrawArrays(GL_LINES, 0, comets.size() * 2 );
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  
  glDisable(GL_BLEND);
}

void MoveCamera(int pIdx)
{
  float m[16];
  openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
  openGLMatrix.LoadIdentity();
  openGLMatrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
  openGLMatrix.Rotate(landRotate[0], 1, 0, 0);
  openGLMatrix.Rotate(landRotate[1], 0, 1, 0);
  openGLMatrix.Rotate(landRotate[2], 0, 0, 1);
  openGLMatrix.Scale(landScale[0], landScale[1], landScale[2]);
  
  cameraPos = glm::vec3(pointPos[curIndex].x-0.01*pointNorm[curIndex].x,pointPos[curIndex].y-0.01*pointNorm[curIndex].y, pointPos[curIndex].z-0.01*pointNorm[curIndex].z);
  openGLMatrix.LookAt(cameraPos.x, cameraPos.y, cameraPos.z,
                      pointPos[curIndex].x+pointTan[curIndex].x,  pointPos[curIndex].y+pointTan[curIndex].y,  pointPos[curIndex].z+pointTan[curIndex].z,
                      -pointNorm[curIndex].x, -pointNorm[curIndex].y, -pointNorm[curIndex].z);
  openGLMatrix.GetMatrix(m);
  float p[16];
  openGLMatrix.SetMatrixMode(OpenGLMatrix::Projection);
  openGLMatrix.GetMatrix(p);
  switch(pIdx)
  {
    case 1:
    {
      glUniformMatrix4fv(h_modelViewMatrix, 1, GL_FALSE, m);
      glUniformMatrix4fv(h_projectionMatrix, 1, GL_FALSE, p);
      break;
    }
    case 3:
    {
      glUniformMatrix4fv(sky_modelViewMatrix, 1, GL_FALSE, m);
      glUniformMatrix4fv(sky_projectionMatrix, 1, GL_FALSE, p);
      break;
    }
    case 4:
    {
      glUniformMatrix4fv(sphere_modelViewMatrix, 1, GL_FALSE, m);
      glUniformMatrix4fv(sphere_projectionMatrix, 1, GL_FALSE, p);
      break;
    }
    case 5:
    {
      glUniformMatrix4fv(comet_modelViewMatrix, 1, GL_FALSE, m);
      glUniformMatrix4fv(comet_projectionMatrix, 1, GL_FALSE, p);
      break;
    }
  }
}

void displayFunc()
{
  // render some stuff...
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  skyProgram.Bind();
  MoveCamera(3);
  renderSky();

  pipelineProgram.Bind();
  MoveCamera(1);
  glUniform3f(t_cameraPos, cameraPos.x, cameraPos.y, cameraPos.z);
  glUniform3f(t_lightPos, lightPos.x, lightPos.y, lightPos.z);
  if (envState == DAY)
    glUniform3f(t_lightColor, lightColor_day.x, lightColor_day.y, lightColor_day.z);
  else
    glUniform3f(t_lightColor, lightColor_night.x, lightColor_night.y, lightColor_night.z);
  glUniform3f(t_objectColor, objectColor.x, objectColor.y, objectColor.z);
  //renderSpline();
  renderCrossSection();
  
  // render moon or sun
  if (envState == NIGHT)
  {
    sphereProgram.Bind();
    MoveCamera(4);
    renderSphere();
  }
  
  // render comets
  if (envState == NIGHT)
  {
    cometProgram.Bind();
    MoveCamera(5);
    renderComet();
  }
  
  glutSwapBuffers();
}

void initPointPos()
{
  for (int i = 0; i<numSplines; i++)
  {
//    for (int j = 1; j<splines[i].numControlPoints-2; j++)
//    {
//      cout << splines[i].points[j].x<<" "<< splines[i].points[j].y << " " << splines[i].points[j].z <<endl;
//    }
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
              s,      -s,  0, 0
      );
      
      for (int k = 0; k<pointPerInterval; k++)
      {
        float u = k*(1.0/(pointPerInterval-1));
        glm::vec4 uu(u*u*u, u*u, u, 1);
        glm::vec3 point =  glm::transpose(control) * glm::transpose(basis) * uu;
        pointPos.push_back(point);
      }
    }
  }
}

void initPointTan()
{
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
              s,      -s,  0, 0
      );
  
      for (int k = 0; k<pointPerInterval; k++) {
        float u = k * (1.0 / (pointPerInterval - 1));
        glm::vec4 uu(3 * u * u, 2 * u, 1, 0);
        glm::vec3 tan = glm::normalize(glm::transpose(control) * glm::transpose(basis) * uu);
        pointTan.push_back(tan);
      }
    }
  }
}

void initPointNorm()
{
  // generate norm and binorm
  glm::vec3 arb(0.0,1.0,0.0);
  glm::vec3 n0 = glm::normalize(glm::cross(pointTan[0],arb));
  glm::vec3 b0 = glm::normalize(glm::cross(pointTan[0],n0));
  pointNorm.push_back(n0);
  pointBino.push_back(b0);
  
  for (int i = 1; i<pointPos.size(); i++)
  {
    glm::vec3 n = glm::normalize(glm::cross(pointBino[i-1], pointTan[i]));
    glm::vec3 b = glm::normalize(glm::cross(pointTan[i], n));
    pointNorm.push_back(n);
    pointBino.push_back(b);
  }
}

void initCrossSection()
{
  float ratio = 0.01;
  float alpha = 0.001;
  float interval = 0.01;
  for (int i = 1; i<pointPos.size()-1; i++)
  {
    glm::vec3 p0 = pointPos[i];
    glm::vec3 n0 = pointNorm[i];
    glm::vec3 b0 = pointBino[i];
  
    glm::vec3 p1 = pointPos[i+1];
    glm::vec3 n1 = pointNorm[i+1];
    glm::vec3 b1 = pointBino[i+1];
  
    // generate the 8 points of the cross section
    glm::vec3 vv[8];
    vv[0] = p0 + alpha * (-n0 + b0);
    vv[1] = p0 + alpha * (n0 + b0);
    vv[2] = p0 + alpha * (n0 - b0);
    vv[3] = p0 + alpha * (-n0 - b0);
    vv[4] = p1 + alpha * (-n1 + b1);
    vv[5] = p1 + alpha * (n1 + b1);
    vv[6] = p1 + alpha * (n1 - b1);
    vv[7] = p1 + alpha * (-n1 - b1);
    
  
    int idx[24] = {0,4,5, 0,1,5,
                   1,5,6, 1,2,6,
                   3,7,6, 3,2,6,
                   0,4,7, 0,3,7};
    
    glm::vec3 faceNorm[4] = {-pointBino[i], pointNorm[i], pointBino[i], -pointNorm[i]};
  
    for (int t=0; t<24; t++)
    {
      // left track point
      glm::vec3 lv = vv[idx[t]] + ratio*((idx[t]<=3) ? pointBino[i]:pointBino[i+1]);
      leftTrack.push_back(lv);
      // right track point
      glm::vec3 rv = vv[idx[t]] - ratio*((idx[t]<=3) ? pointBino[i]:pointBino[i+1]);
      rightTrack.push_back(rv);
      
      //left and right track norm
      leftTrackNormal.push_back(faceNorm[t/6]);
      rightTrackNormal.push_back(faceNorm[t/6]);
    }
  }

  //middle track point and its index
  glm::vec3 prev = pointPos[0];
  for (int i = 1; i<pointPos.size(); i = i+1)
  {
    glm::vec3 curr = pointPos[i];
    if (glm::distance(prev, curr) < interval)
      continue;

    glm::vec3 p0 = pointPos[i] + ratio*pointBino[i];
    glm::vec3 p1 = pointPos[i] - ratio*pointBino[i];
    glm::vec3 n = pointNorm[i];
    glm::vec3 t = pointTan[i];

    glm::vec3 vv[8];
    float beta = 0.001/2;
    vv[0] = p0 + beta * (-n - t);
    vv[1] = p0 + beta * (n - t);
    vv[2] = p0 + beta * (n + t);
    vv[3] = p0 + beta * (-n + t);
    vv[4] = p1 + beta * (-n - t);
    vv[5] = p1 + beta * (n - t);
    vv[6] = p1 + beta * (n + t);
    vv[7] = p1 + beta * (-n + t);
  
    int idx[24] = {0,4,5, 0,1,5,
                   1,5,6, 1,2,6,
                   3,7,6, 3,2,6,
                   0,4,7, 0,3,7};
  
    glm::vec3 faceNorm[4] = {-pointTan[i], pointNorm[i], pointTan[i], -pointNorm[i]};
  
    for (int t=0; t<24; t++)
    {
      middleTrack.push_back(vv[idx[t]]);
      middleTrackNormal.push_back(faceNorm[t/6]);
    }
    prev = curr;
  }
}

void initPiplineProgram()
{
  pipelineProgram.Init("../Shader/trackShader");
  pipelineProgram.Bind();
  trackPH = pipelineProgram.GetProgramHandle();
  h_modelViewMatrix = glGetUniformLocation(trackPH, "modelViewMatrix");
  h_projectionMatrix = glGetUniformLocation(trackPH, "projectionMatrix");
  
  t_cameraPos = glGetUniformLocation(trackPH, "cameraPos");
  t_lightPos = glGetUniformLocation(trackPH, "lightPos");
  t_posLoc = glGetAttribLocation(trackPH, "t_position");
  t_texLoc = glGetAttribLocation(trackPH, "t_texCoord");
  t_normLoc = glGetAttribLocation(trackPH, "t_normal");
  
  t_objectColor = glGetUniformLocation(trackPH, "objectColor");
  t_lightColor = glGetUniformLocation(trackPH, "lightColor");
}

void initSphereProgram()
{
  sphereProgram.Init("../Shader/sphereShader");
  sphereProgram.Bind();
  spherePH= sphereProgram.GetProgramHandle();
  sphere_modelViewMatrix= glGetUniformLocation(spherePH, "modelViewMatrix");
  sphere_projectionMatrix = glGetUniformLocation(spherePH, "projectionMatrix");
  
  h_posLoc =glGetAttribLocation(spherePH, "t_position");
  h_normLoc = glGetAttribLocation(spherePH, "t_normal");
}

void initSkyProgram()
{
  skyProgram.Init("../Shader/skyShader");
  skyProgram.Bind();
  skyPH = skyProgram.GetProgramHandle();
  sky_modelViewMatrix = glGetUniformLocation(skyPH, "s_modelViewMatrix");
  sky_projectionMatrix = glGetUniformLocation(skyPH, "s_projectionMatrix");
  s_posLoc = glGetAttribLocation(skyPH, "s_position");
}

void initCometProgram()
{
  cometProgram.Init("../Shader/cometShader");
  cometProgram.Bind();
  cometPH = cometProgram.GetProgramHandle();
  comet_modelViewMatrix = glGetUniformLocation(cometPH, "c_modelViewMatrix");
  comet_projectionMatrix = glGetUniformLocation(cometPH, "c_projectionMatrix");
  c_posLoc = glGetAttribLocation(cometPH, "c_position");
  c_posColor = glGetAttribLocation(cometPH, "c_color");
}

void initCrossSectionObject()
{
  // make the VAO
  glGenVertexArrays(1, &CSVAO);
  glBindVertexArray(CSVAO);
  // left part
  glGenBuffers(1, &leftVBO);
  glBindBuffer(GL_ARRAY_BUFFER, leftVBO);
  // GLsizeiptr size = leftTrack.size() * sizeof(glm::vec3) + trackTexCorrd.size() * sizeof(glm::vec2) + leftTrackNormal.size() * sizeof(glm::vec3);
  GLsizeiptr size = leftTrack.size() * sizeof(glm::vec3) + leftTrackNormal.size() * sizeof(glm::vec3);
  glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, leftTrack.size() * sizeof(glm::vec3), leftTrack.data());
  //glBufferSubData(GL_ARRAY_BUFFER, leftTrack.size() * sizeof(glm::vec3), trackTexCorrd.size() * sizeof(glm::vec2), trackTexCorrd.data());
  glBufferSubData(GL_ARRAY_BUFFER, leftTrack.size() * sizeof(glm::vec3), leftTrackNormal.size() * sizeof(glm::vec3), leftTrackNormal.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  // right part
  glGenBuffers(1, &rightVBO);
  glBindBuffer(GL_ARRAY_BUFFER, rightVBO);
  //size = rightTrack.size() * sizeof(glm::vec3) + trackTexCorrd.size() * sizeof(glm::vec2) + rightTrackNormal.size() * sizeof(glm::vec3);
  size = rightTrack.size() * sizeof(glm::vec3) + rightTrackNormal.size() * sizeof(glm::vec3);
  glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, rightTrack.size() * sizeof(glm::vec3), rightTrack.data());
  //glBufferSubData(GL_ARRAY_BUFFER, rightTrack.size() * sizeof(glm::vec3), trackTexCorrd.size() * sizeof(glm::vec2), trackTexCorrd.data());
  glBufferSubData(GL_ARRAY_BUFFER, rightTrack.size() * sizeof(glm::vec3), rightTrackNormal.size() * sizeof(glm::vec3), rightTrackNormal.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  glBindVertexArray(0);
  
  // make the middle
  glGenVertexArrays(1, &CSVAO2);
  glBindVertexArray(CSVAO2);
  glGenBuffers(1, &middleVBO);
  glBindBuffer(GL_ARRAY_BUFFER, middleVBO);
  glBufferData(GL_ARRAY_BUFFER, middleTrack.size() * sizeof(glm::vec3) + middleTrackNormal.size() * sizeof(glm::vec3), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, middleTrack.size() * sizeof(glm::vec3), middleTrack.data());
  glBufferSubData(GL_ARRAY_BUFFER, middleTrack.size() * sizeof(glm::vec3), middleTrackNormal.size() * sizeof(glm::vec3), middleTrackNormal.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void initSpherePoint()
{
  float sphereRadius = 0.15;
  int hLine = 15; // must be odd
  int vLine = 16;
  
  // init point
  // top point
  spherePos.push_back(glm::vec3(0, sphereRadius, 0));
  // middle points
  for (int i = 0; i<hLine; i++)
  {
    float degree =  (i+1)*(180/(hLine+1));
    float planeRadius = sphereRadius * sin(degree * PI / 180);
    float pointY = sphereRadius * cos(degree * PI / 180);
    
    for (int j = 0; j<vLine; j++)
    {
      float planeDegree = j * (360/vLine);
      float pointZ = planeRadius * sin(planeDegree * PI / 180);
      float pointX = planeRadius * cos((planeDegree+180) * PI / 180);
      spherePos.push_back(glm::vec3(pointX, pointY, pointZ));
    }
  }
  //bottom point
  spherePos.push_back(glm::vec3(0, -sphereRadius, 0));
  
  int pointNum = spherePos.size();
  // point norm
  for (int i =  0; i<pointNum; i++)
  {
    sphereNorm.push_back(glm::vec3(spherePos[i].x, spherePos[i].y, spherePos[i].z));
  }
  
  //move the point
  for (int i =  0; i<pointNum; i++)
  {
    glm::vec3 v(spherePos[i].x, spherePos[i].y, spherePos[i].z);
    glm::mat4 mt = glm::translate( glm::vec3(-lightPos.x, -lightPos.y, -lightPos.z));
    glm::vec4 new_pos = mt * glm::vec4(v,1.0);
    spherePos[i].x = new_pos.x;
    spherePos[i].y = new_pos.y;
    spherePos[i].z = new_pos.z;
  }
  
  // init index
  // top
  for (int i = 1; i <= vLine-1; i++)
  {
    sphereIndex.push_back(0);
    sphereIndex.push_back(i);
    sphereIndex.push_back(i+1);
  }
  sphereIndex.push_back(0);
  sphereIndex.push_back(vLine);
  sphereIndex.push_back(1);
  
  // middle
  for (int i = 1; i<= (hLine-1)*vLine; i = i+vLine)
  {
    for (int j = 0; j<=vLine-2; j++)
    {
      int p[] = {i +j, i+1 +j, i+vLine +j, i+vLine+1 +j};
      sphereIndex.push_back(p[0]);
      sphereIndex.push_back(p[1]);
      sphereIndex.push_back(p[2]);

      sphereIndex.push_back(p[1]);
      sphereIndex.push_back(p[2]);
      sphereIndex.push_back(p[3]);
    }
    int p[] = {i+vLine-1, i, i+2*vLine-1, i+vLine};
    sphereIndex.push_back(p[0]);
    sphereIndex.push_back(p[1]);
    sphereIndex.push_back(p[2]);

    sphereIndex.push_back(p[1]);
    sphereIndex.push_back(p[2]);
    sphereIndex.push_back(p[3]);
  }

  // bottom
  for (int i = (hLine-1)*vLine; i <= hLine*vLine-1; i++)
  {
    sphereIndex.push_back(pointNum-1);
    sphereIndex.push_back(i);
    sphereIndex.push_back(i+1);
  }
  sphereIndex.push_back(pointNum-1);
  sphereIndex.push_back(hLine*vLine);
  sphereIndex.push_back((hLine-1)*vLine);
}

void initSphereObject()
{
  glGenVertexArrays(1, &sphereVAO);
  // left part
  glGenBuffers(1, &sphereVBO);
  glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
  glBufferData(GL_ARRAY_BUFFER, spherePos.size() * sizeof(glm::vec3) + sphereNorm.size() * sizeof(glm::vec3), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, spherePos.size() * sizeof(glm::vec3), spherePos.data());
  glBufferSubData(GL_ARRAY_BUFFER, spherePos.size() * sizeof(glm::vec3), sphereNorm.size() * sizeof(glm::vec3), sphereNorm.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  // make the EBO
  glGenBuffers(1, &sphereEBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndex.size() * sizeof(int), sphereIndex.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  
  glBindVertexArray(0);
  
  // make texture
  vector<char *> faces =
          {
                  "../Texture/moonbox/right.jpg",
                  "../Texture/moonbox/left.jpg",
                  "../Texture/moonbox/bottom.jpg",
                  "../Texture/moonbox/top.jpg",
                  "../Texture/moonbox/front.jpg",
                  "../Texture/moonbox/back.jpg",
          };
  moon_texture = loadCubeMap(faces);
}

void initSkyObject()
{
  float scale = 1.3;
  float v[] = {
          // positions
          -1.0f,  1.0f, -1.0f,
          -1.0f, -1.0f, -1.0f,
          1.0f, -1.0f, -1.0f,
          1.0f, -1.0f, -1.0f,
          1.0f,  1.0f, -1.0f,
          -1.0f,  1.0f, -1.0f,
          
          -1.0f, -1.0f,  1.0f,
          -1.0f, -1.0f, -1.0f,
          -1.0f,  1.0f, -1.0f,
          -1.0f,  1.0f, -1.0f,
          -1.0f,  1.0f,  1.0f,
          -1.0f, -1.0f,  1.0f,
          
          1.0f, -1.0f, -1.0f,
          1.0f, -1.0f,  1.0f,
          1.0f,  1.0f,  1.0f,
          1.0f,  1.0f,  1.0f,
          1.0f,  1.0f, -1.0f,
          1.0f, -1.0f, -1.0f,
          
          -1.0f, -1.0f,  1.0f,
          -1.0f,  1.0f,  1.0f,
          1.0f,  1.0f,  1.0f,
          1.0f,  1.0f,  1.0f,
          1.0f, -1.0f,  1.0f,
          -1.0f, -1.0f,  1.0f,
          
          -1.0f,  1.0f, -1.0f,
          1.0f,  1.0f, -1.0f,
          1.0f,  1.0f,  1.0f,
          1.0f,  1.0f,  1.0f,
          -1.0f,  1.0f,  1.0f,
          -1.0f,  1.0f, -1.0f,
          
          -1.0f, -1.0f, -1.0f,
          -1.0f, -1.0f,  1.0f,
          1.0f, -1.0f, -1.0f,
          1.0f, -1.0f, -1.0f,
          -1.0f, -1.0f,  1.0f,
          1.0f, -1.0f,  1.0f
  };
  
  for (int i = 0; i< 6*6*3; i=i+3)
  {
    v[i] *= scale;
  }
  
  // make VBO VAO
  glGenVertexArrays(1, &skyVAO);
  glBindVertexArray(skyVAO);
  glGenBuffers(1, & skyVBO);
  glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
  
  glEnableVertexAttribArray(s_posLoc);
  glVertexAttribPointer(s_posLoc, 3, GL_FLOAT, GL_FALSE, 3* sizeof(float), (void*)0);
  
  // make texture
  vector<char *> faces =
  {
    "../Texture/skybox-day/right.jpg",
    "../Texture/skybox-day/left.jpg",
    "../Texture/skybox-day/bottom.jpg",
    "../Texture/skybox-day/top.jpg",
    "../Texture/skybox-day/front.jpg",
    "../Texture/skybox-day/back.jpg"
  };
  
  day_texture = loadCubeMap(faces);
  
    faces =
          {
                  "../Texture/skybox-night/right.jpg",
                  "../Texture/skybox-night/left.jpg",
                  "../Texture/skybox-night/bottom.jpg",
                  "../Texture/skybox-night/top.jpg",
                  "../Texture/skybox-night/front.jpg",
                  "../Texture/skybox-night/back.jpg"
          };
    night_texture = loadCubeMap(faces);
}

void initComet()
{
  int comet_num = 100;
  float y0 = 1.3;
  for (int i = 0; i<comet_num; i++)
  {
    glm::vec3 startPt = glm::vec3(RandomValue(-y0,y0), y0, RandomValue(-y0,y0));
    glm::vec3 direction = glm::normalize(glm::vec3(-1, RandomValue(-2,-1) , 0));
    float t = - y0 / direction.y;
    glm::vec3 endPt = glm::vec3(startPt.x + t*direction.x, -1, startPt.z + t*direction.z);
    
    float speed = RandomValue(5e-5, 5e-4);
    float len = RandomValue(0.05, 0.15);
    Comet c = Comet(endPt, startPt, -direction, speed, len);
    comets.push_back(c);
  
    cometPos.push_back(comets[i].start);
    cometPos.push_back(comets[i].end);
  
    cometColor.push_back(1);
    cometColor.push_back(0);
  }
  
  begin_time = chrono::steady_clock::now();
}

void initCometObject()
{
  glGenVertexArrays(1, &cometVAO);
  glGenBuffers(1, &cometVBO);
  glBindBuffer(GL_ARRAY_BUFFER, cometVBO);
  glBufferData(GL_ARRAY_BUFFER, comets.size() * 2 * (sizeof(glm::vec3) + sizeof(int)), NULL, GL_DYNAMIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, cometPos.size() * sizeof(glm::vec3), cometPos.data());
  glBufferSubData(GL_ARRAY_BUFFER, cometPos.size() * sizeof(glm::vec3), cometColor.size() * sizeof(int), cometColor.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
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
  initPointPos();
  initPointTan();
  initPointNorm();
  initCrossSection();

  initPiplineProgram();
  initCrossSectionObject();
  
  initSkyProgram();
  initSkyObject();
  
  initSphereProgram();
  initSpherePoint();
  initSphereObject();
  
  initComet();
  initCometObject();
  initCometProgram();
}

int loadCubeMap(vector<char*> faces)
{
  unsigned int tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
  
  for (int i = 0; i<faces.size(); i++)
  {
    // read the texture image
    ImageIO img;
    ImageIO::fileFormatType imgFormat;
    ImageIO::errorType err = img.load(faces[i], &imgFormat);
  
    if (err != ImageIO::OK)
    {
      printf("Loading texture from %s failed.\n", faces[i]);
      return -1;
    }
  
    // check that the number of bytes is a multiple of 4
    if (img.getWidth() * img.getBytesPerPixel() % 4)
    {
      printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", faces[i]);
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
          {
            pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
            if (numChannels == 1)
            {
              pixelsRGBA[4 * (h * width + w) + 1] = img.getPixel(w, h, c);
              pixelsRGBA[4 * (h * width + w) + 2] = img.getPixel(w, h, c);
              pixelsRGBA[4 * (h * width + w) + 3] = img.getPixel(w, h, c);
            }
          }
        }
        
     glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  return tex;
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
  //glutIdleFunc(idleFunc);
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
  
  // delay between frames
  glutTimerFunc(waitBeforeMove, timerFunc, curIndex);
  
  // sink forever into the glut loop
  glutMainLoop();
  
  
  return 0;
}

