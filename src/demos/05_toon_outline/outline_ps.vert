#version 460

out vec2 texcoord;
out vec3 view_space_dir;

uniform mat4 clip_to_view;

void main()
{
	     texcoord       = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
	vec4 pos            = vec4(texcoord * 2.0 - 1.0, 0.0, 1.0);
	     view_space_dir = (clip_to_view * pos).xyz;

	gl_Position = pos;
}