#version 460 core
out vec4 frag_color;

in vec3 normal;
in vec3 world_pos;
in vec2 texcoord;

uniform vec3 light_color;
uniform vec3 light_direction;
uniform float light_intensity;
uniform float ambient_factor;
uniform vec3 object_color;

uniform vec3 cam_pos;

uniform float specular_power;
uniform float specular_intensity;

uniform sampler2D texture_diffuse1;

/* Code inspiration: https://roystan.net/articles/toon-shader.html */

/* Something is off with this shader... Maybe adding tonemapping will improve visuals... ? */

uniform vec3 rim_color;
uniform float rim_threshold;
uniform float rim_amount;

const vec3 specular_color = vec3(0.9);

vec4 toonShadingRim(vec3 normal)
{
    vec3 dir_to_eye  = normalize(cam_pos - world_pos);
    vec3 half_vector = normalize(dir_to_eye - light_direction);
    
    /* Calculate diffuse factor */
    float df = dot(normal, -light_direction);
    float sf = dot(normal, half_vector);

    float diffuse = smoothstep(0.0, 0.01, df);
    vec3  light   = light_color * diffuse;

    /* Calculate specular factor */
          sf        = pow(sf * diffuse, specular_power * specular_power);
    float sf_smooth = smoothstep(0.005, 0.01, sf);
    vec3 specular_color = sf_smooth * specular_color * specular_intensity;

    /* Calculate rim lighting */
    float rim_dot       = 1.0 - max(dot(-dir_to_eye, normal), 0.0);
    float rim_intensity = rim_dot * pow(df, rim_threshold);
          rim_intensity = smoothstep(rim_amount - 0.01, rim_amount + 0.01, rim_intensity);
    vec3  rim           = rim_intensity * rim_color;

    vec3 texture_sample = texture(texture_diffuse1, texcoord).rgb;
    vec3 ambient_color  = vec3(ambient_factor);

    vec3 color = light_intensity * texture_sample * object_color * (light + ambient_color + rim + specular_color);

    return vec4(color, 1.0);
}

void main()
{
    frag_color = toonShadingRim(normalize(normal));
} 