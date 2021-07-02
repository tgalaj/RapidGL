#version 460 core

layout(location = 0) in vec3  in_position;
layout(location = 1) in vec2  in_texcoord;
layout(location = 2) in vec3  in_normal;
layout(location = 4) in vec4  in_weights;
layout(location = 5) in ivec4 in_bone_ids;

layout(location = 0) out vec2 v_texcoord;
layout(location = 1) out vec3 v_normal;

const int MAX_BONES = 100;

uniform mat4 mvp;
uniform mat4 model;
uniform mat2x4 bones[MAX_BONES];

void main()
{
	mat2x4 dq0 = bones[in_bone_ids[0]];
	mat2x4 dq1 = bones[in_bone_ids[1]];
	mat2x4 dq2 = bones[in_bone_ids[2]];
	mat2x4 dq3 = bones[in_bone_ids[3]];

	/* Antipodality correction. */
	if(dot(dq0[0], dq1[0]) < 0.0) dq1 *= -1.0;
	if(dot(dq0[0], dq2[0]) < 0.0) dq2 *= -1.0;
	if(dot(dq0[0], dq3[0]) < 0.0) dq3 *= -1.0;

	mat2x4 blended_dq = dq0 * in_weights[0];
	blended_dq       += dq1 * in_weights[1];
	blended_dq       += dq2 * in_weights[2];
	blended_dq       += dq3 * in_weights[3];
	
	float len = length(blended_dq[0]);
	blended_dq /= len;
	
	vec3 local_position = in_position.xyz + 2.0*cross(blended_dq[0].yzw, cross(blended_dq[0].yzw, in_position.xyz) + blended_dq[0].x*in_position.xyz);
	vec3 trans          = 2.0*(blended_dq[0].x*blended_dq[1].yzw - blended_dq[1].x*blended_dq[0].yzw + cross(blended_dq[0].yzw, blended_dq[1].yzw));
	
	local_position += trans;
	gl_Position     = mvp * vec4(local_position, 1.0);

	vec3 local_normal = in_normal + 2.0*cross(blended_dq[0].yzw, cross(blended_dq[0].yzw, in_normal) + blended_dq[0].x*in_normal);
	v_normal          = (model * vec4(local_normal, 0.0)).xyz;

	v_texcoord = in_texcoord;
}