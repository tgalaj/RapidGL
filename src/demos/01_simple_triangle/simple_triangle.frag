#version 460 core
out vec4 frag_color;

uniform vec3 triangle_color;

void main()
{
    frag_color = vec4(triangle_color, 1.0f);
} 