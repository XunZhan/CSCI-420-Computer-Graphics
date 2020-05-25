/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: <Your name here>
 * *************************
*/

/* **************************
 * Environment
 1. Camera is placed at origin (0,0,0)
 2. Focal Length is set to 1
 * *************************
*/

#ifdef WIN32
  #include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
  #include <GL/gl.h>
  #include <GL/glut.h>
#elif defined(__APPLE__)
  #include <OpenGL/gl.h>
  #include <GLUT/glut.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits>
#include <imageIO.h>
#include <algorithm>
#include <iostream>
#include <random>

#ifdef WIN32
  #define strcasecmp _stricmp
#endif

#define MAX_TRIANGLES 2000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

#define EPSILON 1e-5
#define PI 3.14159

// TODO change these parameters for different effect
#define WIDTH 640
#define HEIGHT 480
#define MAX_REFLECTION 3
#define REFLECT_RATIO 0.1
#define ANTI_ALIASING true
#define SOFT_SHADOW true
#define SUB_LIGHTS 30

char * filename = NULL;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int mode = MODE_DISPLAY;

//the field of view of the camera
#define fov 60.0

using namespace std;

unsigned char buffer[HEIGHT][WIDTH][3];
double cell_width, cell_height;
double x_min, y_min;

struct Vec {
    double x, y, z;                  // position, also color (r,g,b)
    Vec(double x_=0, double y_=0, double z_=0){ x=x_; y=y_; z=z_; }
    Vec operator+(const Vec &b) const { return Vec(x+b.x,y+b.y,z+b.z); }
    Vec operator-(const Vec &b) const { return Vec(x-b.x,y-b.y,z-b.z); }
    Vec operator-() const { return Vec(-x,-y,-z); }
    Vec operator*(double b) const { return Vec(x*b,y*b,z*b); }
    Vec operator/(double b) const { return Vec(x/b,y/b,z/b); }
    Vec mult(const Vec &b) const { return Vec(x*b.x,y*b.y,z*b.z); }
    Vec& norm(){ return *this = *this * (1/sqrt(x*x+y*y+z*z)); }
    double dot(const Vec &b) const { return x*b.x+y*b.y+z*b.z; }
    double _2norm() {return sqrt(x*x + y*y + z*z);}
    Vec cross(const Vec&b) const {return Vec(y*b.z-z*b.y,z*b.x-x*b.z,x*b.y-y*b.x);}
};

struct Vertex
{
  Vec position;
  Vec color_diffuse;
  Vec color_specular;
  Vec normal;
  double shininess;
};

struct Triangle
{
  Vertex v[3];
};

struct Sphere
{
  Vec position;
  Vec color_diffuse;
  Vec color_specular;
  double shininess;
  double radius;
};

struct Light
{
  Vec position;
  Vec color;
};

struct Ray
{
    Vec o, d;
};

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
Vec ambient_light;
Vec background_color = Vec(1.0,1.0,1.0);

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b);
inline void UpdateProgress(float progress);

inline double RandomValue()
{
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_real_distribution<double> dist(0.f, 1);
  
  return dist(rng);
}

void InitRays( int row, int col, Ray (&r)[4])
{
  
  double x = x_min + (2*row+1)*cell_width/2.0f;
  double y = y_min + (2*col+1)*cell_height/2.0f;
  double z = -1;
  
  r[0] = {Vec(0,0,0),  Vec(x-cell_width/4.0f,y,z).norm()};
  r[1] = {Vec(0,0,0),  Vec(x+cell_width/4.0f,y,z).norm()};
  r[2] = {Vec(0,0,0),  Vec(x,y-cell_height/4.0f,z).norm()};
  r[3] = {Vec(0,0,0),  Vec(x,y+cell_height/4.0f,z).norm()};
}

void initLights()
{
  if (!SOFT_SHADOW)
    return;
  
  // add sub lights
  int origin_light_num = num_lights;
  
  for (int i = 0; i<origin_light_num; i++)
  {
    Vec color = lights[i].color/SUB_LIGHTS;
    Vec center = lights[i].position;
    
    lights[i].color = color;
    for (int j = 0; j<(SUB_LIGHTS-1); j++)
    {
      lights[num_lights].color = color;
      lights[num_lights].position = Vec (center.x+RandomValue(), center.y+RandomValue(), center.z+RandomValue());
      num_lights++;
    }
  }
  return;
}

Vec Clamp(Vec &color)
{
  if (color.x > 1.0)
    color.x = 1.0;
  if (color.y > 1.0)
    color.y = 1.0;
  if (color.z > 1.0)
    color.z = 1.0;
}

bool InRange(double v, double low, double high)
{
  return (v >= low && v <= high);
}

Vec ReflectDir (const Vec &I, const Vec &N) //I and N should be normalized
{
  Vec reflect = I - N * 2 * N.dot(I);
  return reflect.norm();
}

Vec RefractDir  (const Vec &I, const Vec &N, const double &ratio) //I and N should be normalized
{
  double k = 1.0 - ratio * ratio * (1.0 - N.dot(I) * N.dot(I));
  if (k < 0.0)
    return Vec(0,0,0) ;
  else
    return I * ratio - N * (ratio * N.dot(I) + sqrt(k));
}

/*-----------------------------------------------------
 *                     Sphere
 -----------------------------------------------------*/
bool SphereIntersect(const Ray &r, double &t, int &idx)
{
  t = numeric_limits<double>::max();
  idx = -1;
  for (int i = 0; i<num_spheres; i++)
  {
    Vec oc = Vec(r.o-spheres[i].position);
    double a = 1;
    double b = 2 * oc.dot(r.d);
    double c = oc.dot(oc) - pow(spheres[i].radius,2);
    double delta = b*b - 4*a*c;
    if (delta < 0) continue;
    double t1 = (-b + sqrt(delta))/2;
    double t2 = (-b - sqrt(delta))/2;
    double _t = min(t1,t2);
    if (_t > EPSILON && _t<t)
    {
      t = _t;
      idx = i;
    }
  }
  return (idx != -1);
}

/*-----------------------------------------------------
 *                      Triangle
 -----------------------------------------------------*/
bool TriangleIntersect(const Ray &r, double &t, int &idx)
{
  t = numeric_limits<double>::max();
  idx = -1;
  for (int i = 0; i<num_triangles; i++)
  {
    // Möller Trumbore Algorithm
    Vec e1 = triangles[i].v[1].position - triangles[i].v[0].position;
    Vec e2 = triangles[i].v[2].position - triangles[i].v[0].position;
    Vec s = r.o - triangles[i].v[0].position;
    Vec s1 = r.d.cross(e2);
    Vec s2 = s.cross(e1);
    double k = s1.dot(e1);
    if (k > -EPSILON && k < EPSILON)
      continue;
    double _t = 1/k * s2.dot(e2);
    double b1 = 1/k * s1.dot(s);
    double b2 = 1/k * s2.dot(r.d);
    double b3 = 1-b1-b2;
    if (_t < EPSILON || !InRange(b1, 0, 1) || !InRange(b2, 0, 1) || !InRange(b3, 0, 1))
      continue;
    if (_t < t)
    {
      t = _t;
      idx = i;
    }
  }
  
  return (idx != -1);
}

void CalcColorRatio(int &idx, Vec pos, double (&ratio)[3])
{
  Vec ab = triangles[idx].v[1].position - triangles[idx].v[0].position;
  Vec ac = triangles[idx].v[2].position - triangles[idx].v[0].position;
  double ABC_Area = ab.cross(ac)._2norm() * 0.5f;
  Vec pa = triangles[idx].v[0].position - pos;
  Vec pb = triangles[idx].v[1].position - pos;
  Vec pc = triangles[idx].v[2].position - pos;
  
  double PBC_Area = pb.cross(pc)._2norm() * 0.5f;
  double PCA_Area = pc.cross(pa)._2norm() * 0.5f;
  double PAB_Area = pa.cross(pb)._2norm() * 0.5f;
  
  ratio[0] = PBC_Area / ABC_Area;
  ratio[1] = PCA_Area / ABC_Area;
  ratio[2] = PAB_Area / ABC_Area;
}

/*-----------------------------------------------------
 *                     Ray Tracing
 -----------------------------------------------------*/
bool InShadow(const Light &light, const Vertex &point)
{
  Vec dir = (light.position-point.position).norm();
  Ray shadowRay = {point.position + dir * 5 * EPSILON, dir};
  
  double t1, t2; int i1, i2;
  bool hitSphere = SphereIntersect(shadowRay, t1, i1);
  bool hitTriangle = TriangleIntersect(shadowRay, t2, i2);
  
  // hit nothing
  if (!hitSphere && !hitTriangle)
    return false;
  
  // hit point should not exceed light position
  Vec hitPos;
  if ((hitSphere && !hitTriangle) || (hitSphere && hitTriangle && t1<t2))
    hitPos =  shadowRay.o + shadowRay.d * t1;
  else
    hitPos =  shadowRay.o + shadowRay.d * t2;

  double len_point2light = (point.position - light.position)._2norm();
  double len_point2hit = (point.position - hitPos)._2norm();
  if (len_point2hit - len_point2light > EPSILON)
    return false;
  
  return true;
}

Vec CalcShading (Vertex &hitPoint, Light &light)
{
  Vec diffuse, specular;
  Vec lightDir = (light.position-hitPoint.position).norm();
  double diff = max(lightDir.dot(hitPoint.normal),0.0);
  diffuse = light.color.mult(hitPoint.color_diffuse * diff);
  
  // camera is at origin
  Vec viewDir = (-hitPoint.position).norm();
  Vec reflectDir = ReflectDir(-lightDir, hitPoint.normal);
  double spec = pow(max(viewDir.dot(reflectDir),0.0), hitPoint.shininess);
  specular =  light.color.mult(hitPoint.color_specular * spec);
  
  return diffuse + specular;
}

Vec CalcRadiance(const Ray &r, int times)
{
  double t1,t2; int idx1,idx2;
  bool hitSphere = SphereIntersect(r, t1, idx1);
  bool hitTriangle = TriangleIntersect(r,t2,idx2);
  
  // hit nothing
  if (!hitSphere && !hitTriangle)
    return background_color;
  
  Vertex hitPoint;
  // hit sphere
  if ((hitSphere && !hitTriangle) || (hitSphere && hitTriangle && t1<t2))
  {
    hitPoint = {
            r.o + r.d * t1,
            spheres[idx1].color_diffuse,
            spheres[idx1].color_specular,
            (r.o + r.d * t1 - spheres[idx1].position).norm(),
            spheres[idx1].shininess
    };
  }
  // hit triangle
  else
  {
    double ratio[3];
    Vec position = r.o + r.d * t2;
    CalcColorRatio(idx2, position, ratio);
    Vec diffuse = triangles[idx2].v[0].color_diffuse * ratio[0] +
            triangles[idx2].v[1].color_diffuse * ratio[1] +
            triangles[idx2].v[2].color_diffuse * ratio[2];
  
    Vec specular = triangles[idx2].v[0].color_specular * ratio[0] +
                   triangles[idx2].v[1].color_specular * ratio[1] +
                   triangles[idx2].v[2].color_specular * ratio[2];
  
    Vec normal = triangles[idx2].v[0].normal * ratio[0] +
                   triangles[idx2].v[1].normal * ratio[1] +
                   triangles[idx2].v[2].normal * ratio[2];
    
    double shininess = triangles[idx2].v[0].shininess * ratio[0] +
                   triangles[idx2].v[1].shininess * ratio[1] +
                   triangles[idx2].v[2].shininess * ratio[2];
    hitPoint = {
            position,
            diffuse,
            specular,
            normal.norm(),
            shininess
    };
  }
  
  Vec curRayColor = Vec(0,0,0);
  // for each light
  for (int k = 0; k<num_lights; k++)
  {
    if (!InShadow(lights[k], hitPoint))
    {
      curRayColor =  curRayColor + CalcShading(hitPoint, lights[k]);
    }
  }
  
  // the last recursive ray
  if (times >= MAX_REFLECTION)
    return curRayColor;
  
  times++;
  // reflect Ray
  Vec reflectDir = ReflectDir(r.d, hitPoint.normal);
  Ray reflectRay = {hitPoint.position, reflectDir};
  Vec reflectColor = CalcRadiance(reflectRay, times);
  
  return curRayColor * (1-REFLECT_RATIO) +  reflectColor * REFLECT_RATIO;
}

void InitScreen()
{
  double aspectRatio = (double)WIDTH/HEIGHT;
  
  float rad = fov * PI / 180;
  double screen_width = 2*aspectRatio*tan(rad/2);
  double screen_height = 2*tan(rad/2);
  cell_width =  screen_width/WIDTH;
  cell_height = screen_height/HEIGHT;
  x_min = -aspectRatio*tan(rad/2);
  y_min = -tan(rad/2);
}

//MODIFY THIS FUNCTION
void draw_scene()
{
  InitScreen();
  //a simple test output
  for(unsigned int x=0; x<WIDTH; x++)
  {
    glPointSize(2.0);  
    glBegin(GL_POINTS);
    for(unsigned int y=0; y<HEIGHT; y++)
    {
      if (ANTI_ALIASING)
      {
        Ray rays[4];
        Vec color;
        InitRays(x,y,rays);
        for (int k = 0; k<4; k++)
        {
          color = color + CalcRadiance(rays[k], 0);
        }
        color = color/4;
        plot_pixel(x, y, (int)(color.x * 255), (int)(color.y * 255), (int)(color.z * 255));
      }
      else
      {
        Ray ray;
        Vec color;
        double xx = x_min + (2*x+1)*cell_width/2.0f;
        double yy = y_min + (2*y+1)*cell_height/2.0f;
        double zz = -1;
  
        ray = {Vec(0,0,0),  Vec(xx,yy,zz).norm()};
        color = CalcRadiance(ray, 1) + ambient_light;
        Clamp(color);
        plot_pixel(x, y, (int)(color.x * 255), (int)(color.y * 255), (int)(color.z * 255));
      }
      
    }
    UpdateProgress(x / (float)WIDTH);
    glEnd();
    glFlush();
  }
  printf("Done!\n"); fflush(stdout);
}

inline void UpdateProgress(float progress)
{
  int barWidth = 30;
  
  std::cout << "[";
  int pos = barWidth * progress;
  for (int i = 0; i < barWidth; ++i) {
    if (i < pos) std::cout << "=";
    else if (i == pos) std::cout << ">";
    else std::cout << " ";
  }
  std::cout << "] " << int(progress * 100.0) << " %\r";
  std::cout.flush();
};

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
  glVertex2i(x,y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  buffer[y][x][0] = r;
  buffer[y][x][1] = g;
  buffer[y][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
    plot_pixel_jpeg(x,y,r,g,b);
}

void save_jpg()
{
  printf("Saving JPEG file: %s\n", filename);

  ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
  if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
    printf("Error in Saving\n");
  else 
    printf("File saved Successfully\n");
}

void parse_check(const char *expected, char *found)
{
  if(strcasecmp(expected,found))
  {
    printf("Expected '%s ' found '%s '\n", expected, found);
    printf("Parse error, abnormal abortion\n");
    exit(0);
  }
}

void parse_doubles(FILE* file, const char *check, Vec &p)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p.x,&p.y,&p.z);
  //printf("%s %lf %lf %lf\n",check,p.x,p.y,p.z);
}

void parse_rad(FILE *file, double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  //printf("rad: %f\n",*r);
}

void parse_shi(FILE *file, double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  //printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE * file = fopen(argv,"r");
  int number_of_objects;
  char type[50];
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i", &number_of_objects);

  printf("number of objects: %i\n",number_of_objects);

  parse_doubles(file,"amb:",ambient_light);

  for(int i=0; i<number_of_objects; i++)
  {
    fscanf(file,"%s\n",type);
    //printf("%s\n",type);
    if(strcasecmp(type,"triangle")==0)
    {
      //printf("found triangle\n");
      for(int j=0;j < 3;j++)
      {
        parse_doubles(file,"pos:",t.v[j].position);
        parse_doubles(file,"nor:",t.v[j].normal);
        parse_doubles(file,"dif:",t.v[j].color_diffuse);
        parse_doubles(file,"spe:",t.v[j].color_specular);
        parse_shi(file,&t.v[j].shininess);
      }

      if(num_triangles == MAX_TRIANGLES)
      {
        printf("too many triangles, you should increase MAX_TRIANGLES!\n");
        exit(0);
      }
      triangles[num_triangles++] = t;
    }
    else if(strcasecmp(type,"sphere")==0)
    {
      //printf("found sphere\n");

      parse_doubles(file,"pos:",s.position);
      parse_rad(file,&s.radius);
      parse_doubles(file,"dif:",s.color_diffuse);
      parse_doubles(file,"spe:",s.color_specular);
      parse_shi(file,&s.shininess);

      if(num_spheres == MAX_SPHERES)
      {
        printf("too many spheres, you should increase MAX_SPHERES!\n");
        exit(0);
      }
      spheres[num_spheres++] = s;
    }
    else if(strcasecmp(type,"light")==0)
    {
      //printf("found light\n");
      parse_doubles(file,"pos:",l.position);
      parse_doubles(file,"col:",l.color);

      if(num_lights == MAX_LIGHTS)
      {
        printf("too many lights, you should increase MAX_LIGHTS!\n");
        exit(0);
      }
      lights[num_lights++] = l;
    }
    else
    {
      printf("unknown type in scene description:\n%s\n",type);
      exit(0);
    }
  }
  return 0;
}
void display()
{
}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
  //hack to make it only draw once
  static int once=0;
  if(!once)
  {
    initLights();
    draw_scene();
    if(mode == MODE_JPEG)
      save_jpg();
  }
  once=1;
}

int main(int argc, char ** argv)
{
  if ((argc < 2) || (argc > 3))
  {  
    printf ("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
    exit(0);
  }
  if(argc == 3)
  {
    mode = MODE_JPEG;
    filename = argv[2];
  }
  else if(argc == 2)
    mode = MODE_DISPLAY;

  glutInit(&argc,argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(WIDTH - 1, HEIGHT - 1);
  #endif
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
}

