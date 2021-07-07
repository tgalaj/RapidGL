#version 460 core
layout (location = 0) in vec3 in_pos;
layout (location = 2) in vec3 in_normal;

uniform mat4 mvp;
uniform mat4 model_matrix;

out vec3 world_pos;
out vec3 normal;

void main()
{

    world_pos = vec3(model_matrix * vec4(in_pos,   1.0));
    normal    = vec3(model_matrix * vec4(in_normal, 0.0));

    gl_Position = mvp * vec4(in_pos, 1.0);
}