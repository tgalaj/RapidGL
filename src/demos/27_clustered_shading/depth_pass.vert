#version 460
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_texcoord;

layout(location = 0) out vec2 texcoord;

uniform mat4 mvp;

void main()
{
	texcoord    = in_texcoord;
	gl_Position = mvp * vec4(in_pos, 1.0);
}