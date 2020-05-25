#version 150

in vec3 Position;
in vec3 Normal;
//in vec2 TexCoord;
in vec3 cameraPosition;
in vec3 lightPosition;

out vec4 c;

//uniform sampler2D tTexture;
uniform samplerCube tTexture;

uniform vec3 lightColor;
uniform vec3 objectColor;

void main()
{
    // compute the final pixel color
    float ratio = 1.00/1.52;
    float colRatio = 0.8;
    vec3 I = normalize(Position - cameraPosition);
    vec3 R = refract(I, normalize(Normal), ratio);

    //c = vec4(newcolor, 1.0);

    float alpha  = 32;
    float ambientStrength = 0.3;
    float specularStrength = 0.4;

    vec3 lightDir = normalize(lightPosition - Position);
    vec3 reflectDirc = reflect(-lightDir, Normal);
    vec3 viewDir = normalize(cameraPosition - Position);

    //
    vec3 ambient = ambientStrength * lightColor;
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * lightColor;
    float spec = pow(max(dot(viewDir, reflectDirc),0.0), alpha);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 color = (ambient + diffuse + specular) * objectColor;
    vec3 newcolor = colRatio*texture(tTexture, R).rgb + (1-colRatio)*color;
    c = vec4(newcolor, 1.0);

//    c = texture(tTexture, TexCoord);
}

