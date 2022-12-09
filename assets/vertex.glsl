#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 view;
uniform mat4 proj;

out vec2 texCoord;
out vec4 vertexColor;
out vec3 normal;

void main()
{
    mat4 mvp = proj * view;
    gl_Position = mvp * vec4(aPos, 1.0);
    texCoord = aTexCoord;
    vertexColor = vec4(0.9, 0.9, 0.9, 1.0);
    normal = aNorm;
}