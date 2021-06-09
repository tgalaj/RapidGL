#version 460 core

layout(location = 0) out vec4 frag_color;

uniform int should_keep_color;

in float transparency_FS_in;
in vec2 texcoord_FS_in;

layout(binding = 0) uniform sampler2D particle_texture;

void main()
{
    frag_color    = texture(particle_texture, texcoord_FS_in);
    frag_color    = vec4(mix(vec3(0.0), frag_color.rgb, max(transparency_FS_in, should_keep_color)), frag_color.a);
    frag_color.a *= transparency_FS_in;
}