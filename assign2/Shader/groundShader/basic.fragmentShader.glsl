#version 150

out vec4 FragColor;

//in vec3 ourColor;
in vec2 g_TexCoord;

uniform sampler2D ourTexture;

void main()
{
  FragColor = texture(ourTexture, g_TexCoord);
}