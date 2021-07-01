#version 460 core
const vec3 light_dirs[6] = { vec3(-1,  0,  0),
                             vec3( 1,  0,  0),
                             vec3( 0,  1,  0),
                             vec3( 0, -1,  0),
                             vec3( 0,  0,  1),
                             vec3( 0,  0, -1) };

out vec4 frag_color;

layout(location = 0) in vec2 v_texcoord;
layout(location = 1) in vec3 v_normal;

layout(binding = 0) uniform sampler2D tex_diffuse;

uniform float gamma;

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
    vec4 albedo_color = texture(tex_diffuse, v_texcoord);
    frag_color = vec4(0.0);

    for(int i = 0; i < 6; ++i)
    {
        frag_color += max(0.0, dot(normalize(v_normal), normalize(light_dirs[i])));
    }

    frag_color *= albedo_color * 10.0;
    frag_color += vec4(0.18, 0.18, 0.18, 1.0);

    frag_color = reinhard(frag_color);
} 