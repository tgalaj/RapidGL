#version 460 core
layout (location = 0) in vec3 in_pos;
layout (location = 2) in vec3 in_normal;

uniform mat4 mvp;
uniform mat3 mvp_normal;
uniform vec2 screen_resolution;

uniform float outline_width;

void main()
{
    vec4 clip_position = mvp * vec4(in_pos, 1.0);
    vec3 clip_normal   = mvp_normal * in_normal;
    vec2 offset        = normalize(clip_normal.xy) / screen_resolution * outline_width * clip_position.w * 2.0;

    clip_position.xy += offset;

    gl_Position = clip_position;
}