#version 460

layout (location = 0) in vec3 in_pos;

uniform mat4 u_model;
uniform mat4 u_light_matrix;

void main()
{
	gl_Position = u_light_matrix * u_model * vec4(in_pos, 1.0);
}