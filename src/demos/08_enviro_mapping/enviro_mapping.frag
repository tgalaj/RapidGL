#version 450

out vec4 frag_color;

in vec3 world_pos;
in vec3 world_normal;

layout(binding = 1) uniform samplerCube skybox;
uniform vec3 cam_pos;
uniform float ior;

subroutine vec4 enviroMapping();
layout(location = 0) subroutine uniform enviroMapping enviro_func;

layout(index = 0) subroutine(enviroMapping) vec4 reflection()
{
    vec3 i = normalize(world_pos - cam_pos);
    vec3 r = reflect(i, normalize(world_normal));

    return vec4(texture(skybox, r).rgb, 1.0);
}

layout(index = 1) subroutine(enviroMapping) vec4 refraction()
{
    vec3 ratio = 1.0 / (vec3(ior) + vec3(0.0, 0.01, 0.02));
    vec3 i = normalize(world_pos - cam_pos);
    
    vec3 refract_dir_r = refract(i, normalize(world_normal), ratio.r);
    vec3 refract_dir_g = refract(i, normalize(world_normal), ratio.g);
    vec3 refract_dir_b = refract(i, normalize(world_normal), ratio.b);
    vec3 reflect_dir   = reflect(i, normalize(world_normal));

    float refract_color_r = texture(skybox, refract_dir_r).r;
    float refract_color_g = texture(skybox, refract_dir_g).g;
    float refract_color_b = texture(skybox, refract_dir_b).b;

    vec4 refract_color = vec4(refract_color_r, refract_color_g, refract_color_b, 1.0);
    vec4 reflect_color = texture(skybox, reflect_dir);

    float alpha = max(0.0, dot(i, normalize(world_normal)));

    return mix(refract_color, reflect_color, alpha);
}

void main()
{
    frag_color = enviro_func();
}