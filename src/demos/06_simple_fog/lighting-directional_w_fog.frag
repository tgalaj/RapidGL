#version 460 core
#include "../../src/demos/03_lighting/lighting.glh"

uniform DirectionalLight directional_light;

uniform float ambient_factor;

uniform float fog_min_distance;
uniform float fog_max_distance;
uniform float fog_density;
uniform vec3  fog_color;

subroutine float FogEquation(float distance_to_camera);           /* Subroutine Type     */
layout(location = 0) subroutine uniform FogEquation fog_equation; /* Subroutine uniform  */

layout(index = 0) subroutine(FogEquation) float fog_factor_linear(float distance_to_camera)
{
    return (fog_max_distance - distance_to_camera) / (fog_max_distance - fog_min_distance);
}

layout(index = 1) subroutine(FogEquation) float fog_factor_exp(float distance_to_camera)
{
    return exp(-fog_density * distance_to_camera);
}

layout(index = 2) subroutine(FogEquation) float fog_factor_exp2(float distance_to_camera)
{
    float exponent = fog_density * distance_to_camera;
    return exp(-exponent * exponent);
}

void main()
{
    vec4 ambient                        = texture(texture_diffuse1, texcoord) * vec4(vec3(ambient_factor), 1.0);
    vec4 directional_light_contribution = calcDirectionalLight(directional_light, normalize(normal), world_pos);

    float distance_to_cam = distance(world_pos, cam_pos);
    float fog_factor = fog_equation(distance_to_cam);
          fog_factor = clamp(fog_factor, 0.0, 1.0);

    vec4 light_color = reinhard(ambient + directional_light_contribution);
    vec3 color = mix(fog_color, light_color.rgb, fog_factor);

    frag_color = vec4(color, 1.0);
} 