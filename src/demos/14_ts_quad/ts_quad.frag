#version 460 core

layout(location = 0) out vec4 frag_color;

noperspective in vec3 g_edge_distance;

uniform vec3 quad_color;
uniform vec3 line_color;
uniform float line_width;

void main()
{
    // Find the smallest distance
    float min_d = min(min(g_edge_distance.x, g_edge_distance.y), g_edge_distance.z);

    // Determine the mix factor with the line color
    float mix_value = smoothstep(line_width - 1, line_width + 1, min_d);

	frag_color = mix(vec4(line_color, 1.0), vec4(quad_color, 1.0), mix_value);
}