#version 450

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

out vec3 world_pos;
out vec3 world_normal;

uniform mat4 g_mvp;
uniform mat4 g_model;
uniform mat3 g_normal_matrix;

void main()
{
    world_pos     = (g_model * vec4(a_position, 1.0f)).xyz;
    world_normal  = normalize(g_normal_matrix * a_normal);

    gl_Position = g_mvp * vec4(a_position, 1.0f);
}