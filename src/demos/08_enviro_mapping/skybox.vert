#version 450
layout(location = 0) in vec3 position;

out vec3 o_texcoord;

uniform mat4 view_projection;

void main()
{
	o_texcoord  = position;
	vec4 pos    = view_projection * vec4(position, 1.0f);
	gl_Position = pos.xyww;
}