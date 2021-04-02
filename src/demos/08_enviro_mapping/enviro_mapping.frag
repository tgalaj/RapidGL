#version 450

out vec4 frag_color;

in vec3 world_pos;
in vec3 world_normal;

layout(binding = 0) uniform samplerCube skybox;
uniform vec3 g_cam_pos;

subroutine vec4 enviroMapping();
layout(location = 0) subroutine uniform enviroMapping enviro_func;

layout(index = 0) subroutine(enviroMapping) vec4 reflection()
{
    vec3 i = normalize(world_pos - g_cam_pos);
    vec3 r = reflect(i, normalize(world_normal));

    float ratio = 1.0f / 1.52f;
    vec3 r_r = refract(i, normalize(world_normal), ratio);

    return mix(vec4(texture(skybox, r).rgb, 1.0f), vec4(texture(skybox, r_r).rgb, 1.0f), 0.5);
}

layout(index = 1) subroutine(enviroMapping) vec4 refraction()
{
    float ratio = 1.0f / 1.52f;
    vec3 i = normalize(world_pos - g_cam_pos);
    vec3 r = refract(i, normalize(world_normal), ratio);

    return vec4(texture(skybox, r).rgb, 1.0f);
}

void main()
{
    frag_color = enviro_func();
}