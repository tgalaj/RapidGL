#version 460 core
layout (location = 0) out vec4 frag_normal_depth;

in vec3 view_pos;
in vec3 view_normal;

void main()
{
    vec3 normal = normalize(view_normal) * 0.5 + 0.5;
    float depth = gl_FragCoord.z;

    frag_normal_depth = vec4(normal, depth);
}