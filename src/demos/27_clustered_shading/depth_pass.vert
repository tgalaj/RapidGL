#version 460
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexcoord;

layout(location = 0) out vec2 texcoord;

uniform mat4 mvp;

void main()
{
	texcoord    = aTexcoord;
	gl_Position = mvp * vec4(aPos, 1.0);
}