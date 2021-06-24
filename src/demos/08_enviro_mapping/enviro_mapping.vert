#version 450

layout(location = 0) in vec3 a_position;
layout(location = 2) in vec3 a_normal;

out vec3 world_pos;
out vec3 world_normal;

uniform mat4 mvp;
uniform mat4 model;
uniform mat3 normal_matrix;

void main()
{
    world_pos     = (model * vec4(a_position, 1.0)).xyz;
    world_normal  = normalize(normal_matrix * a_normal);

    gl_Position = mvp * vec4(a_position, 1.0);
}