#version 150

out vec4 FragColor;

//in vec3 ourColor;
in vec3 s_TexCoord;

uniform samplerCube skybox;

void main()
{
  FragColor = texture(skybox, s_TexCoord);
}