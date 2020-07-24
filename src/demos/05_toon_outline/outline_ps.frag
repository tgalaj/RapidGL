#version 460

/* Code courtesy of: https://roystan.net/articles/outline-shader.html */

in vec2 texcoord;
in vec3 view_space_dir;

out vec4 fragColor;

layout(binding = 0) uniform sampler2D normals_depth_texture;
layout(binding = 1) uniform sampler2D main_texture;

uniform float depth_threshold;
uniform float depth_normal_threshold;
uniform float depth_normal_threshold_scale;
uniform float normal_threshold;

uniform float outline_width;
uniform vec3  outline_color;

vec4 alpha_blend(vec4 top, vec4 bottom)
{
	vec3  color = (top.rgb * top.a) + (bottom.rgb * (1 - top.a));
	float alpha = top.a + bottom.a * (1 - top.a);

	return vec4(color, alpha);
}

void main()
{
	float half_width_scale_floor = floor(outline_width);
	float half_width_scale_ceil  = ceil (outline_width);

	vec2 pixel_size = 1.0 / textureSize(normals_depth_texture, 0);

	vec4 normal_depth = texture(normals_depth_texture, texcoord);

	/* Texcoords */
	vec2 bottom_left  = texcoord - pixel_size * half_width_scale_floor;
	vec2 top_right    = texcoord + pixel_size * half_width_scale_ceil;
	vec2 bottom_right = texcoord + vec2( pixel_size.x * half_width_scale_ceil, -pixel_size.y * half_width_scale_floor);
	vec2 top_left     = texcoord + vec2(-pixel_size.x * half_width_scale_floor, -pixel_size.y * half_width_scale_ceil);;

	/* Normal edge */
	vec3 normal0 = texture(normals_depth_texture, bottom_left).rgb;	 // BL
	vec3 normal1 = texture(normals_depth_texture, top_right).rgb;	 // TR
	vec3 normal2 = texture(normals_depth_texture, bottom_right).rgb; // BR
	vec3 normal3 = texture(normals_depth_texture, top_left).rgb;     // TL

	/* Depth edge */
	float depth0 = texture(normals_depth_texture, bottom_left).a;  // BL
	float depth1 = texture(normals_depth_texture, top_right).a;	   // TR
	float depth2 = texture(normals_depth_texture, bottom_right).a; // BR
	float depth3 = texture(normals_depth_texture, top_left).a;     // TL

	/* Return a value from [0, 1] range to [-1, 1] range. */
	vec3 view_normal = normal_depth.rgb * 2.0 - 1.0;
	float NdotV = 1.0 - dot(view_normal, -view_space_dir);

	float n_threshold = clamp((NdotV - depth_normal_threshold) / (1.0001 - depth_normal_threshold), 0.0, 1.0);
	      n_threshold = n_threshold * depth_normal_threshold_scale + 1.0;

	float d_threshold = depth_threshold * normal_depth.a * n_threshold;

	float depht_finite_diff0 = depth1 - depth0;
	float depht_finite_diff1 = depth3 - depth2;

	// edgeDepth is calculated using the Roberts cross operator.
	// The same operation is applied to the normal below.
	// https://en.wikipedia.org/wiki/Roberts_cross
	float edge_depth = sqrt(pow(depht_finite_diff0, 2) + pow(depht_finite_diff1, 2)) * 100.0;
		  edge_depth = edge_depth > d_threshold ? 1.0 : 0.0;

	vec3 normal_finite_diff0 = normal1 - normal0;
	vec3 normal_finite_diff1 = normal3 - normal2;

	float edge_normal = sqrt(dot(normal_finite_diff0, normal_finite_diff0) + dot(normal_finite_diff1, normal_finite_diff1));
		  edge_normal = edge_normal > normal_threshold ? 1.0 : 0.0;

	float edge = max(edge_depth, edge_normal);

	vec4 edge_color = vec4(outline_color, 1.0 * edge);
	vec4 color = texture(main_texture, texcoord);

	fragColor = alpha_blend(edge_color, color);
}