#version 150

in vec3 t_position;
in vec3 t_normal;
//in vec2 t_texCoord;

out vec3 Position;
out vec3 Normal;
//out vec2 TexCoord;
out vec3 cameraPosition;
out vec3 lightPosition;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform vec3 cameraPos;
uniform vec3 lightPos;

void main()
{
    // compute the transformed and projected vertex position (into gl_Position)
    // compute the vertex color (into col)
    gl_Position = projectionMatrix * modelViewMatrix * vec4(t_position, 1.0f);

    mat3 normalMatrix = mat3(transpose(inverse(modelViewMatrix)));
    Normal = normalize(normalMatrix * t_normal);
    Position = vec3(modelViewMatrix * vec4(t_position,  1.0f));
    // TexCoord = t_texCoord;

    cameraPosition = vec3(modelViewMatrix * vec4(cameraPos, 1.0f));
    lightPosition = vec3(modelViewMatrix * vec4(lightPos, 1.0f));
}

