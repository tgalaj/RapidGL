#version 460 core
layout (location = 0) in vec3 in_pos;
layout (location = 2) in vec3 in_normal;

uniform mat4 view_model;
uniform mat4 projection;
uniform mat3 normal_matrix_view_space;

out vec3 view_normal;

void main()
{
    view_normal = normal_matrix_view_space * in_normal;

    gl_Position = projection * view_model * vec4(in_pos, 1.0);
}