#version 450

out vec2 texcoord;

void main()
{
	texcoord    = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
	gl_Position = vec4(texcoord * 2.0f - 1.0f, 0.0f, 1.0f);
}