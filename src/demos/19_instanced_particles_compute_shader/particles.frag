#version 460 core

layout(location = 0) out vec4 frag_color;

layout(location = 0) in vec3 v_normal;

const vec3 light_dir = vec3(1, 1, 1);

uniform vec3 u_diffuse;

void main()
{
    float d      = dot(normalize(v_normal), normalize(light_dir));
    vec3 diffuse = d * u_diffuse;

    frag_color = vec4(0.18 + diffuse, 1.0);
}