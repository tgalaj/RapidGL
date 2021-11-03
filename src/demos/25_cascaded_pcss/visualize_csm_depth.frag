#version 460 core

in vec2 texcoord;
out vec4 frag_color;

uniform int u_layer;

layout(binding = 0) uniform sampler2DArray filterTexture;

vec4 colors[3] = vec4[]( vec4(1,0,0,1), vec4(0,1,0,1), vec4(0,0,1,1) );

void main()
{
	frag_color = vec4(texture(filterTexture, vec3(texcoord, u_layer)).xxx, 1.0);
}