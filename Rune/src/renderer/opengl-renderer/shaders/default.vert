#version 450

layout(location = 0) in vec3 input_position;
layout(location = 0) out vec3 color;

vec3 colours[3] = {
    vec3(1.f, 0.f, 0.f),
    vec3(0.f, 1.f, 0.f),
    vec3(0.f, 0.f, 1.f)
};

void main()
{
    gl_Position = vec4(input_position, 1.);
    color = vec3(1) /* colours[gl_VertexIndex] */;
}
