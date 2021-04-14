#version 460 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

out vec3 g_normal;
out vec3 g_position;
noperspective out vec3 g_edge_distance;

in vec3 world_pos[];
in vec3 normal[];

uniform mat4 viewport_matrix;

void main()
{
	// Transform each vertex into viewport space
    vec2 p0 = vec2(viewport_matrix * (gl_in[0].gl_Position / gl_in[0].gl_Position.w));
    vec2 p1 = vec2(viewport_matrix * (gl_in[1].gl_Position / gl_in[1].gl_Position.w));
    vec2 p2 = vec2(viewport_matrix * (gl_in[2].gl_Position / gl_in[2].gl_Position.w));

    // Calc the lengths of the triangle edges
    float a = length(p1 - p2);
    float b = length(p2 - p0);
    float c = length(p1 - p0);

    // Calc triangle angles (using law of cosines)
    float alpha = acos( (b*b + c*c - a*a) / (2.0*b*c) );
    float beta = acos( (a*a + c*c - b*b) / (2.0*a*c) );

    // Calc triangle altitudes
    float ha = abs( c * sin( beta ) );
    float hb = abs( c * sin( alpha ) );
    float hc = abs( b * sin( alpha ) );

    g_edge_distance = vec3( ha, 0, 0 );
    g_normal        = normal[0];
    g_position      = world_pos[0];
    gl_Position     = gl_in[0].gl_Position;
    EmitVertex();

    g_edge_distance = vec3( 0, hb, 0 );
    g_normal        = normal[1];
    g_position      = world_pos[1];
    gl_Position     = gl_in[1].gl_Position;
    EmitVertex();

    g_edge_distance = vec3( 0, 0, hc );
    g_normal        = normal[2];
    g_position      = world_pos[2];
    gl_Position     = gl_in[2].gl_Position;
    EmitVertex();

	EndPrimitive();
}