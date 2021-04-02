#version 450
in vec3 o_texcoord;

out vec4 fragcolor;

layout(binding = 0) uniform samplerCube cubeSampler;

void main()
{
	fragcolor = texture(cubeSampler, o_texcoord);
}