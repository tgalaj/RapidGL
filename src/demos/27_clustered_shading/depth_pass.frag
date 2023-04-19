#version 460

layout(location = 0) in vec2 texcoord;
layout(binding = 0) uniform sampler2D albedoTexture;

void main()
{
	float alpha = texture(albedoTexture, texcoord).a;

	if (alpha < 0.5) discard;
}