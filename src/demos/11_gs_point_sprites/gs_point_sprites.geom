#version 460 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform float half_quad_width;
uniform mat4 projection_matrix;

out vec2 tex_coords;

void main()
{
	gl_Position = projection_matrix * (vec4(-half_quad_width, -half_quad_width, 0.0, 0.0) + gl_in[0].gl_Position);
	tex_coords = vec2(0.0, 1.0);
	EmitVertex();

	gl_Position = projection_matrix * (vec4(half_quad_width, -half_quad_width, 0.0, 0.0) + gl_in[0].gl_Position);
	tex_coords = vec2(1.0, 1.0);
	EmitVertex();

	gl_Position = projection_matrix * (vec4(-half_quad_width, half_quad_width, 0.0, 0.0) + gl_in[0].gl_Position);
	tex_coords = vec2(0.0, 0.0);
	EmitVertex();

	gl_Position = projection_matrix * (vec4(half_quad_width, half_quad_width, 0.0, 0.0) + gl_in[0].gl_Position);
	tex_coords = vec2(1.0, 0.0);
	EmitVertex();

	EndPrimitive();
}