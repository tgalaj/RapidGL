#version 460 core

layout(location = 0) out vec4 frag_color;

uniform vec3 point_color;

void main()
{
	frag_color = vec4(point_color, 1.0);
}