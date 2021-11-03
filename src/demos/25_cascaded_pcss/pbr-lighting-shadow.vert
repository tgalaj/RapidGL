#version 460 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_texcoord;
layout (location = 2) in vec3 in_normal;

const int NUM_CASCADES = 3;

uniform mat4 u_model;
uniform mat4 u_mvp;
uniform mat4 u_mv;
uniform mat3 u_normal_matrix;
uniform mat4 u_light_view_projections[NUM_CASCADES];
uniform mat4 u_light_views[NUM_CASCADES];

layout (location = 0) out vec2 out_texcoord;
layout (location = 1) out vec3 out_world_pos;
layout (location = 2) out vec3 out_normal;
layout (location = 3) out float out_clip_space_pos_z;
layout (location = 4) out vec4 out_pos_light_view_space[NUM_CASCADES];
layout (location = 7) out vec4 out_pos_light_clip_space[NUM_CASCADES];

void main()
{
    out_world_pos = vec3(u_model * vec4(in_pos, 1.0));
    out_texcoord  = in_texcoord;
    out_normal    = u_normal_matrix * in_normal;

    for (int i = 0; i < NUM_CASCADES; ++i)
    {
        out_pos_light_view_space[i] = u_light_views[i] * vec4(out_world_pos, 1.0);
        out_pos_light_clip_space[i] = u_light_view_projections[i] * vec4(out_world_pos, 1.0);
    }

    gl_Position = u_mvp * vec4(in_pos, 1.0);
    out_clip_space_pos_z = gl_Position.z;
}