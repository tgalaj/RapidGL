#version 460 core
layout (location = 0) in vec3 in_pos;

layout (location = 0) out vec3 out_world_pos;

uniform mat4 u_projection;
uniform mat4 u_view;

void main()
{
	out_world_pos = in_pos;
	gl_Position = u_projection * u_view * vec4(out_world_pos, 1.0);
}