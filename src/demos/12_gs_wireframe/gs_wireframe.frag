#version 460 core
out vec4 frag_color;

in vec3 g_normal;   // world normal
in vec3 g_position; // world position
noperspective in vec3 g_edge_distance;

uniform vec3 cam_pos;
uniform vec3 ambient;

uniform float specular_intensity;
uniform float specular_power;

struct LineInfo
{
    float width;
    vec4 color;
};

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
uniform LineInfo line_info;

void main()
{
    vec4 color = calcDirectionalLight(directional_light, normalize(g_normal), g_position);

    // Find the smallest distance
    float min_d = min(min(g_edge_distance.x, g_edge_distance.y), g_edge_distance.z);

    // Determine the mix factor with the line color
    float mix_value = smoothstep(line_info.width - 1, line_info.width + 1, min_d);

    frag_color = mix(line_info.color, color, mix_value);
}