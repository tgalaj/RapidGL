#version 460 core
#define PI 3.141592653589793238462643

out vec4 frag_color;

layout (location = 0) in vec3 in_world_pos;

layout (binding = 1) uniform samplerCube environment_map;

void main()
{
    vec3 n          = normalize(in_world_pos);
    vec3 irradiance = vec3(0.0);

    // tangent space calculation from the origin point
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, n));
         up    = normalize(cross(n, right));

    float num_steps   = 69;
    float sample_step = 2.0 * PI / num_steps;
    float nr_samples  = 0.0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += sample_step)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sample_step)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangent_sample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

            // tangent space to world
            vec3 sample_vec = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * n;

            irradiance += texture(environment_map, sample_vec).rgb * cos(theta) * sin(theta);
            nr_samples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nr_samples));
    
    frag_color = vec4(irradiance, 1.0);
}