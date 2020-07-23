#version 460 core
out vec4 frag_color;

in vec2 texcoord;
in vec3 world_pos;
in vec3 normal;

uniform float ambient_factor;
uniform float gamma;

uniform sampler2D texture_diffuse1;

vec4 reinhard(vec4 hdr_color)
{
    // reinhard tonemapping
    vec3 ldr_color = hdr_color.rgb / (hdr_color.rgb + 1.0);

    // gamma correction
    ldr_color = pow(ldr_color, vec3(1.0 / gamma));

    return vec4(ldr_color, 1.0);
}

void main()
{
    frag_color = reinhard(texture(texture_diffuse1, texcoord) * vec4(vec3(ambient_factor), 1.0));
} 