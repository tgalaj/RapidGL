#version 460 core
out vec4 frag_color;

layout (location = 0) in vec3 in_world_pos;

layout (binding = 1) uniform sampler2D equirectangular_map;

const vec2 inv_atan = vec2(0.1591, 0.3183);
vec2 sampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= inv_atan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = sampleSphericalMap(normalize(in_world_pos));
    vec3 color = texture(equirectangular_map, uv).rgb;
    
    frag_color = vec4(color, 1.0);
}