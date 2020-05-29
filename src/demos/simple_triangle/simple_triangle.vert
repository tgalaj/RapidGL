#version 460 core
layout (location = 0) in vec3 in_pos;

uniform vec2 triangle_translation;

void main()
{
    gl_Position = vec4(in_pos + vec3(triangle_translation, 0.0), 1.0);
}