#version 460 core

layout(location = 0) out vec4 frag_color;

uniform vec3 line_color;

void main()
{
	frag_color = vec4(line_color, 1.0);
}