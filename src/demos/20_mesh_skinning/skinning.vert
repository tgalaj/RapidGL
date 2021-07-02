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
uniform mat4 bones[MAX_BONES];

void main()
{       
    mat4 bone_transform  = bones[in_bone_ids[0]] * in_weights[0];
         bone_transform += bones[in_bone_ids[1]] * in_weights[1];
         bone_transform += bones[in_bone_ids[2]] * in_weights[2];
         bone_transform += bones[in_bone_ids[3]] * in_weights[3];

    v_texcoord = in_texcoord;

    vec4 local_normal = bone_transform * vec4(in_normal, 0);
    v_normal          = (model * local_normal).xyz;

    vec4 local_position = bone_transform * vec4(in_position, 1.0);
    gl_Position         = mvp * local_position;
}