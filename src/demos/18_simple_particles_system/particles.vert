#version 460 core
const float PI = 3.14159265359;

layout (location = 0) in vec3  in_pos;
layout (location = 1) in vec3  in_velocity;
layout (location = 2) in float in_age;

/* Output to transform feedback buffers - update pass only */
layout( xfb_buffer = 0, xfb_offset=0 ) out vec3  tf_pos;
layout( xfb_buffer = 1, xfb_offset=0 ) out vec3  tf_velocity;
layout( xfb_buffer = 2, xfb_offset=0 ) out float tf_age;

/* Outputs to fragment shader */
layout(location = 0) out float v_transparency;
layout(location = 1) out vec2  v_texcoord;

/* Uniforms */
uniform float delta_t;
uniform vec3 acceleration; // gravity
uniform float particle_lifetime;
uniform vec3 emitter_world_pos;
uniform mat3 emitter_basis; // rotation that rotates y axis to the direction of emitter
uniform vec2 particle_size_min_max;

uniform vec2 start_position_min_max;
uniform vec2 start_velocity_min_max;
uniform vec3 direction_constraints;
uniform float cone_angle;

uniform mat4 model_view;
uniform mat4 projection;

layout(binding = 1) uniform sampler1D random_texture;

// Offsets to the position in camera coordinates for each vertex of the particle's quad
const vec3 offsets[] = vec3[](vec3(-0.5,-0.5,0), vec3(0.5,-0.5,0), vec3(0.5,0.5,0),
                              vec3(-0.5,-0.5,0), vec3(0.5,0.5,0), vec3(-0.5,0.5,0) );

// Texture coordinates for each vertex of the particle's quad
const vec2 texcoords[] = vec2[](vec2(0,0), vec2(1,0), vec2(1,1), vec2(0,0), vec2(1,1), vec2(0,1));

vec3 random_initial_velocity() 
{
    float theta    = mix(0.0,                      cone_angle,               texelFetch(random_texture, 3 * gl_VertexID,     0).r);
    float phi      = mix(0.0,                      2.0 * PI,                 texelFetch(random_texture, 3 * gl_VertexID + 1, 0).r);
    float velocity = mix(start_velocity_min_max.x, start_velocity_min_max.y, texelFetch(random_texture, 3 * gl_VertexID + 2, 0).r);
    vec3  v        = vec3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));

    return normalize(emitter_basis * v * direction_constraints) * velocity;
}

vec3 random_initial_position() 
{
    float offset = mix(start_position_min_max.x, start_position_min_max.y, texelFetch(random_texture, 2 * gl_VertexID + 1, 0).r);
    return emitter_world_pos + vec3(offset, 0, 0);
}

subroutine void render_pass_type();
layout(location = 0) subroutine uniform render_pass_type render_pass;

layout(index = 0) subroutine(render_pass_type) void update()
{
    if (in_age < 0 || in_age > particle_lifetime)
    {
        /* Particle is past it's lifetime - recycle */
        tf_pos      = random_initial_position();
        tf_velocity = random_initial_velocity();
    
        if (in_age < 0) 
            tf_age = in_age + delta_t;
        else 
            tf_age = (in_age - particle_lifetime) + delta_t;
    }
    else
    {
        /* Particle is alive - update it */
        tf_pos      = in_pos + in_velocity * delta_t;
        tf_velocity = in_velocity + acceleration * delta_t;
        tf_age      = in_age + delta_t;
    }
}

layout(index = 1) subroutine(render_pass_type) void render()
{
    v_transparency = 0.0;
    vec3 pos_view_space = vec3(0.0);

    if (in_age >= 0.0)
    {
        float age_pct    = in_age / particle_lifetime;
        v_transparency = clamp(1.0 - age_pct, 0, 1);
        pos_view_space   = (model_view * vec4(in_pos, 1.0)).xyz + offsets[gl_VertexID] * 
                           mix(particle_size_min_max.x, particle_size_min_max.y, age_pct);
    }

    v_texcoord = texcoords[gl_VertexID];

    gl_Position = projection * vec4(pos_view_space, 1);
}

void main()
{
	render_pass();
}