#version 460 core
layout (location = 0) in vec3 in_pos;

uniform mat4 u_projection;
uniform mat4 u_view;

layout (location = 0) out vec3 out_world_pos;

void main()
{
    out_world_pos = in_pos;

	mat4 rot_view = mat4(mat3(u_view));
	vec4 clip_pos = u_projection * u_view * vec4(out_world_pos, 1.0);

	gl_Position = clip_pos.xyww;
}