#version 330 core

out vec4 FragColor;

in vec4 vertexColor;
in vec4 normal;
in vec2 texCoord;

uniform sampler2D t;

void main()
{
    float intensity = (1 + dot(vec3(normal), normalize(vec3(10, 14, 8)))) / 2;
    FragColor = intensity * texture(t, texCoord);
}