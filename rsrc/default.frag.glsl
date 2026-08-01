#version 330 core

out vec4 FragColor;
in vec2 vTexCoord;

uniform sampler2D Tex;

void main()
{
    FragColor = texture(Tex, vTexCoord);
}