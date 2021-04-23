#version 460 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_texcoord;

uniform mat4 model;
uniform mat3 normal_matrix;

out vec3 world_pos_TCS_in;
out vec3 world_normal_TCS_in;
out vec2 texcoord_TCS_in;

void main()
{
	world_pos_TCS_in    = vec3(model * vec4(in_pos, 1.0));
	world_normal_TCS_in = normal_matrix * in_normal;
	texcoord_TCS_in     = in_texcoord.xy;
}