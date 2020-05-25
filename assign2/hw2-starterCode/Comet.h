#ifndef HW2_COMET_H
#define HW2_COMET_H

#include <glm/vec3.hpp>

class Comet
{
public:
    glm::vec3 start;
    glm::vec3 end;
    glm::vec3 direction;
    
    float tmax;
    float speed;
    float len;
    
    Comet(glm::vec3 o, glm::vec3 e, glm::vec3 d, float s, float l)
    {
      start = o;
      end = e;
      direction = d;
      speed = s;
      len = l;
      
      tmax = glm::distance(o,e);
    }
    
};

#endif //HW2_COMET_H
