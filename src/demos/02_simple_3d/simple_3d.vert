#version 460 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_texcoord;

uniform mat4 mvp;

out vec2 out_texcoord;

void main()
{
    out_texcoord = in_texcoord;

    gl_Position = mvp * vec4(in_pos, 1.0);
}