#version 460 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_texcoord;

uniform mat4 model;
uniform mat4 mvp;
uniform mat3 normal_matrix;

out vec3 world_pos;
out vec3 normal;
out vec2 texcoord;

void main()
{
    world_pos = vec3(model * vec4(in_pos, 1.0));
    normal    = normal_matrix * in_normal;
    texcoord  = in_texcoord.st;

    gl_Position = mvp * vec4(in_pos, 1.0);
}