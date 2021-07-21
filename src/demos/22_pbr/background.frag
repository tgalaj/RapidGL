#version 460 core
out vec4 frag_color;

layout (location = 0) in vec3 in_world_pos;

layout (binding = 0) uniform samplerCube environment_map;

void main()
{
    vec3 envColor = textureLod(environment_map, -in_world_pos, 0.0).rgb;
    frag_color = vec4(envColor, 1.0);
}