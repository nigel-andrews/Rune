#version 460

layout (location = 0) in vec3 colour;
layout (location = 0) out vec4 frag_color;

void main()
{
    frag_color = vec4(colour, 1.f);
}
