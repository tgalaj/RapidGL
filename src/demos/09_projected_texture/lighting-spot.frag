#version 460 core
#include "lighting.glh"

uniform SpotLight spot_light;

in vec4 projector_texcoord;

layout(binding = 1) uniform sampler2D projector_texture;

void main()
{
    vec4 light = calcSpotLight(spot_light, normalize(normal), world_pos);

    vec3 projector_texture_color = vec3(0.0);
    if(projector_texcoord.z > 0.0)
    {
        projector_texture_color = textureProj(projector_texture, projector_texcoord).rgb;
    }

    frag_color = reinhard(light + vec4(projector_texture_color, 1.0));
} 