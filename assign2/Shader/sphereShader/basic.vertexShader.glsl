#version 150

in vec3 t_position;
in vec3 t_normal;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

out vec3 Reflect;

void main()
{
    // compute the transformed and projected vertex position (into gl_Position)
    // compute the vertex color (into col)
    gl_Position = projectionMatrix * modelViewMatrix * vec4(t_position, 1.0f);

    Reflect = t_normal;

}

