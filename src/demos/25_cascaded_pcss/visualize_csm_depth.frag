#version 460 core

in vec2 texcoord;
out vec4 frag_color;

uniform int u_layer;

layout(binding = 0) uniform sampler2DArray filterTexture;

void main()
{
	frag_color = vec4(texture(filterTexture, vec3(texcoord, u_layer)).xxx, 1.0);
}