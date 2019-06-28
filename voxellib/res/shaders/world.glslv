#version 330 core
layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec3 vertex_colour;
layout (location = 2) in float vertex_ao;
out vec4 rgba;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    // TODO move proj*view calculation onto cpu
    gl_Position = projection * view * vec4(vertex_pos, 1.0);

    // ao
    const float ambient = 1;
    const vec4 ambient_colour = vec4(ambient, ambient, ambient, 1.0);
    const float ao_scale = 0.65;
    float scaled_ao = ((vertex_ao * ao_scale) + (1-ao_scale));

    rgba = vec4(vertex_colour, 1.0);
    rgba *= ambient_colour * scaled_ao;
}