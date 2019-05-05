#version 330 core
layout (location = 0) in vec3 vertex_pos;
out vec4 rgba;

void main()
{
    gl_Position = vec4(vertex_pos, 1.0);
    rgba = vec4(0.5, 0.8, 0.2, 1.0);
}