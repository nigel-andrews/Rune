#version 460

// layout (location = 0) in vec3 in_pos;

vec3 vertices[3] = {
    vec3(0.f, -0.5f, 0.f),
    vec3(0.5f, 0.5f, 0.f),
    vec3(-0.5f, 0.5f, 0.f)
};

vec3 colours[3] = {
    vec3(1.f, 0.f, 0.f),
    vec3(0.f, 1.f, 0.f),
    vec3(0.f, 0.f, 1.f)
};

layout(location = 0) out vec3 colour;

// TODO: MVP matrix

void main()
{
    gl_Position = vec4(vertices[gl_VertexIndex], 1.f);
    colour = colours[gl_VertexIndex];
}
