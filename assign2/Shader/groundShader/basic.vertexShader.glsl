#version 150

in vec3 g_position;
//in vec3 aColor;
in vec2 g_texCoord;

//out vec3 ourColor;
out vec2 g_TexCoord;

uniform mat4 g_modelViewMatrix;
uniform mat4 g_projectionMatrix;

void main()
{
  gl_Position = g_projectionMatrix * g_modelViewMatrix * vec4(g_position, 1.0);
  //ourColor = aColor;
  g_TexCoord = g_texCoord;
}

