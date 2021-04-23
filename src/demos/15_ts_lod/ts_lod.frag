#version 460 core

layout(location = 0) out vec4 frag_color;

in vec3 world_pos_FS_in;
in vec3 world_normal_FS_in;
in vec2 texcoord_FS_in;

uniform vec3 cam_pos;
uniform vec3 ambient;

uniform float specular_intensity;
uniform float specular_power;

struct BaseLight
{
    vec3 color;
    float intensity;
};

struct DirectionalLight
{
    BaseLight base;
    vec3 direction;
};

vec4 blinnPhong(BaseLight base, vec3 direction, vec3 normal, vec3 world_pos)
{
    float diffuse = max(dot(normal, -direction), 0.0);

    vec3 dir_to_eye  = normalize(cam_pos - world_pos);
    vec3 half_vector = normalize(dir_to_eye - direction);
    float specular   = pow(max(dot(half_vector, normal), 0.0), specular_power);

    vec4 ambient_color  = base.intensity * vec4(ambient, 1.0);
    vec4 diffuse_color  = vec4(base.color, 1.0) * base.intensity * diffuse;
    vec4 specular_color = vec4(1.0) * specular * specular_intensity;

    return ambient_color + diffuse_color + specular_color;
}

vec4 calcDirectionalLight(DirectionalLight light, vec3 normal, vec3 world_pos)
{
    return blinnPhong(light.base, light.direction, normal, world_pos);
}

uniform DirectionalLight directional_light;

void main()
{
    vec4 radiance = calcDirectionalLight(directional_light, normalize(world_normal_FS_in), world_pos_FS_in);

    frag_color = radiance;
}