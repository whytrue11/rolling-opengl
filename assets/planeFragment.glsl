#version 330 core

out vec4 FragColor;

in vec4 vertexColor;
in vec4 normal;
in vec2 texCoord;

uniform sampler2D t;

void main()
{
    FragColor = texture(t, texCoord);
}
