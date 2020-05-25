#version 150

in vec3 c_position;
in int c_color;

out vec4 Color;

uniform mat4 c_modelViewMatrix;
uniform mat4 c_projectionMatrix;

void main()
{
    // compute the transformed and projected vertex position (into gl_Position)
    // compute the vertex color (into col)
    gl_Position = c_projectionMatrix * c_modelViewMatrix * vec4(c_position, 1.0f);

    if (c_color == 0)
    {
        Color = vec4(1.0, 1.0, 1.0, 0.0);
        return ;
    }

    Color = vec4(1.0, 1.0, 1.0, 1.0);
}

