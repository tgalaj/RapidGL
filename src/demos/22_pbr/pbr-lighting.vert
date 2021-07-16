#version 460 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_texcoord;
layout (location = 2) in vec3 in_normal;

uniform mat4 u_model;
uniform mat4 u_mvp;
uniform mat3 u_normal_matrix;

layout (location = 0) out vec2 out_texcoord;
layout (location = 1) out vec3 out_world_pos;
layout (location = 2) out vec3 out_normal;

void main()
{
    out_world_pos = vec3(u_model * vec4(in_pos, 1.0));
    out_texcoord  = in_texcoord;
    out_normal    = u_normal_matrix * in_normal;

    gl_Position = u_mvp * vec4(in_pos, 1.0);
}