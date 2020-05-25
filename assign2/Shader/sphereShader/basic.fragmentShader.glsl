#version 150

in vec3 Reflect;

out vec4 c;

uniform samplerCube tTexture;
//uniform vec3 lightColor;


void main()
{
    // compute the final pixel color
    float a = 0.7;
    vec3 newcolor = a * texture(tTexture, Reflect).rgb + (1-a) * vec3(0.8, 0.9, 1.0);
    c = vec4(newcolor, 1.0);
    //c = vec4(0.9, 0.9, 0.0, 1.0);

}

