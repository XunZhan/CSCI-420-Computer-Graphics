#version 150

in vec3 s_position;

//out vec3 ourColor;
out vec3 s_TexCoord;

uniform mat4 s_modelViewMatrix;
uniform mat4 s_projectionMatrix;

void main()
{
  gl_Position = s_projectionMatrix * s_modelViewMatrix * vec4(s_position, 1.0);
  //ourColor = aColor;
  s_TexCoord = s_position;
}

