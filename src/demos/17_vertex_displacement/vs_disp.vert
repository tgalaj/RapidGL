#version 460 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_texcoord;

uniform mat4 mvp;
uniform mat4 model;
uniform mat3 normal_matrix;

uniform float time;
uniform float amplitude;
uniform float velocity;
uniform float frequency;

out vec3 world_pos_FS_in;
out vec3 world_normal_FS_in;
out vec2 texcoord_FS_in;

void main()
{
	vec4  pos = vec4(in_pos, 1.0);
	float u   = frequency * (pos.x - velocity * time);
	
		 pos.y  = amplitude * sin(u);
	vec3 normal = vec3(-amplitude * frequency * cos(u), 1.0, 0.0);

	world_pos_FS_in    = vec3(model * pos);
	world_normal_FS_in = normalize(normal_matrix * normalize(normal));
	texcoord_FS_in     = in_texcoord.xy;

	gl_Position = mvp * pos;
}