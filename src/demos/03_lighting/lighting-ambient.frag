#version 460 core
out vec4 frag_color;

in vec2 texcoord;
in vec3 world_pos;
in vec3 normal;

uniform float ambient_factor;
uniform sampler2D texture_diffuse1;

void main()
{
    frag_color = texture(texture_diffuse1, texcoord) * vec4(vec3(ambient_factor), 1.0);
} 