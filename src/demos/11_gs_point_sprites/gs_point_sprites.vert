#version 460 core
layout (location = 0) in vec3 in_pos;

uniform mat4 model_view_matrix;

void main()
{
	gl_Position = model_view_matrix * vec4(in_pos, 1.0);
}