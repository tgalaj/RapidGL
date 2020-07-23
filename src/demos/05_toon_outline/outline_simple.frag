#version 460 core
out vec4 frag_color;

uniform vec3 outline_color;

void main()
{
    frag_color = vec4(outline_color, 1.0);
}