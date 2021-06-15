#version 460 core

layout(location = 0) out vec4 frag_color;

uniform int should_keep_color;

layout(location = 0) in float v_transparency;
layout(location = 1) in vec2  v_texcoord;

layout(binding = 0) uniform sampler2D particle_texture;

void main()
{
    frag_color    = texture(particle_texture, v_texcoord);
    frag_color    = vec4(mix(vec3(0.0), frag_color.rgb, max(v_transparency, should_keep_color)), frag_color.a);
    frag_color.a *= v_transparency;
}