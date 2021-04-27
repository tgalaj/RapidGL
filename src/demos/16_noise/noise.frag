#version 460 core
#define PI 3.14159265

out vec4 frag_color;
in  vec2 out_texcoord;

uniform sampler2D texture_diffuse1;

// Cloud
uniform vec3 sky_color;
uniform vec3 cloud_color;

// Wood grain
uniform vec3 dark_wood_color;
uniform vec3 light_wood_color;
uniform mat4 slice_matrix;

// Disintegration
uniform float low_threshold;
uniform float high_threshold;

subroutine vec3 noise_sub();
layout(location = 0) subroutine uniform noise_sub noise_func;

layout(index = 0) subroutine(noise_sub) vec3 cloud()
{
    vec4 noise = texture(texture_diffuse1, out_texcoord);
    float t = (cos(noise.g * PI) + 1.0) * 0.5;

    return mix(sky_color, cloud_color, t);
}

layout(index = 1) subroutine(noise_sub) vec3 wood_grain()
{
    vec4 cyl = slice_matrix * vec4(out_texcoord, 0.0, 1.0);
    float dist = length(cyl.xz);
    
    vec4 noise = texture(texture_diffuse1, out_texcoord);
    dist += noise.b;

    float t = 1.0 - abs(fract(dist) * 2.0 - 1.0);
    t = smoothstep(0.2, 0.5, t);

    return mix(dark_wood_color, light_wood_color, t);
}

layout(index = 2) subroutine(noise_sub) vec3 disintegration()
{
    vec4 noise = texture(texture_diffuse1, out_texcoord);
    
    if(noise.a < low_threshold || noise.a > high_threshold)
    {
        discard;
    }

    return vec3(out_texcoord, 0.0);
}

void main()
{
    vec3 noise_color = noise_func();
    frag_color = vec4(noise_color, 1.0);
} 