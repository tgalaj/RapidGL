#version 460 core
out vec4 frag_color;

in vec2 out_texcoord;

layout(binding = 0) uniform sampler2D texture_diffuse1;
uniform vec3 color;
uniform float mix_factor;

void main()
{
    frag_color = mix(texture(texture_diffuse1, out_texcoord), vec4(color, 1.0), mix_factor);
} 