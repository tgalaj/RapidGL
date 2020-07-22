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

/* Code courtesy of: https://www.shadertoy.com/view/lllyDB */

uniform float diffuse_levels;
uniform float specular_levels;
uniform float light_shade_cutoff;
uniform float dark_shade_cutoff;

const mat4 light_shade_mat = mat4( 251, 166, 10,  165,
                                   142, 9,   212, 250,
                                   7,   165, 250, 168,
                                   220, 250, 142, 4 ) / 255.0;

vec3 light_shade_color()
{        
    ivec2 gridPos = ivec2(gl_FragCoord) % 4;
    float factor  = light_shade_mat[gridPos.x][3 - gridPos.y];

    return vec3(factor);
}

const mat4 dark_shade_mat = mat4( 251, 166, 10,  165,
                                  142, 9,   212, 8,
                                  7,   165, 250, 168,
                                  220, 8,   142, 4 ) / 255.0;
    
vec3 dark_shade_color()
{        
    ivec2 gridPos = ivec2(gl_FragCoord) % 4;
    float factor  = dark_shade_mat[gridPos.x][3 - gridPos.y];

    return vec3(factor);
}

vec4 toonShading(vec3 normal)
{
    vec3 dir_to_eye  = normalize(cam_pos - world_pos);
    vec3 half_vector = normalize(dir_to_eye - light_direction);
    
    float df = max(0.0, dot(normal, -light_direction));
    float sf = max(0.0, dot(normal, half_vector));
          sf = pow(sf, specular_power);

    df = ceil(df * diffuse_levels) / diffuse_levels;
    sf = floor((sf * specular_levels) + 0.5) / specular_levels;

    vec3 diff_color_modulation;

    if (df >= light_shade_cutoff)
    {
        diff_color_modulation = vec3(1.0);
    }
    else if (df >= dark_shade_cutoff)
    {
        diff_color_modulation = light_shade_color();
    }
    else
    {
        diff_color_modulation = dark_shade_color();
    }

    vec3 albedo_texture = texture(texture_diffuse1, texcoord).rgb;
    vec3 ambient_color  = ambient_factor * object_color * albedo_texture;
    vec3 diffuse_color  = light_color * light_intensity * object_color * albedo_texture * diff_color_modulation;
    vec3 specular_color = vec3(specular_intensity) * light_intensity;

    vec3 color = ambient_color + df * diffuse_color + sf * specular_color;

    return vec4(color, 1.0);
}

void main()
{
    frag_color = toonShading(normalize(normal));
} 