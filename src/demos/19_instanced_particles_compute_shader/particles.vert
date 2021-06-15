#version 460 core

/* Mesh data */
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;

/* Particle data */
layout (location = 4) in vec4 in_particle_position;
layout (location = 5) in vec2 in_particle_rotation;

layout(location = 0) out vec3 v_normal;

uniform mat4 u_mvp;
uniform mat4 u_model_view;

void main()
{
	float cosine = cos(in_particle_rotation.x);
	float sine   = sin(in_particle_rotation.x);

	mat4 RT = mat4(1,  0,      0,      0,
				   0,  cosine, sine,   0,
				   0, -sine,   cosine, 0,
				   in_particle_position.x, in_particle_position.y, in_particle_position.z, 1);

	v_normal   = (u_model_view * RT * vec4(in_normal,   0.0)).xyz;

	gl_Position = u_mvp * RT * vec4(in_position, 1.0);
}