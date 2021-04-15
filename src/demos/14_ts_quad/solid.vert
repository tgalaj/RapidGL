#version 460 core
layout (location = 0) in vec2 in_pos;

uniform mat4 view_projection;

void main()
{
	gl_Position = view_projection * vec4(in_pos, 0.0, 1.0);
}