#version 460 core
#include "../../src/demos/03_lighting/lighting.glh"

uniform DirectionalLight directional_light;

uniform float ambient_factor;

uniform float alpha_cutout_threshold;

void main()
{
    vec4 ambient                        = texture(texture_diffuse1, texcoord) * vec4(vec3(ambient_factor), 1.0);
    vec4 directional_light_contribution = calcDirectionalLight(directional_light, normalize(normal), world_pos);

    vec4 light_color = (ambient + directional_light_contribution);

    if(light_color.a <= alpha_cutout_threshold)
        discard;

    light_color = reinhard(light_color);

    frag_color = light_color;
} 