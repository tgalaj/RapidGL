#version 460 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform mat4 viewport_matrix;

in vec3 world_pos_GS_in[];
in vec3 world_normal_GS_in[];
in vec2 texcoord_GS_in[];

out vec3 world_pos_FS_in;
out vec3 world_normal_FS_in;
out vec2 texcoord_FS_in;

noperspective out vec3 edge_distance_FS_in;

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

    world_pos_FS_in     = world_pos_GS_in[0];
    world_normal_FS_in  = world_normal_GS_in[0];
    texcoord_FS_in      = texcoord_GS_in[0];
    edge_distance_FS_in = vec3( ha, 0, 0 );
    gl_Position         = gl_in[0].gl_Position;
    EmitVertex();

    world_pos_FS_in     = world_pos_GS_in[1];
    world_normal_FS_in  = world_normal_GS_in[1];
    texcoord_FS_in      = texcoord_GS_in[1];
    edge_distance_FS_in = vec3( 0, hb, 0 );
    gl_Position         = gl_in[1].gl_Position;
    EmitVertex();

    world_pos_FS_in     = world_pos_GS_in[2];
    world_normal_FS_in  = world_normal_GS_in[2];
    texcoord_FS_in      = texcoord_GS_in[2];
    edge_distance_FS_in = vec3( 0, 0, hc );
    gl_Position         = gl_in[2].gl_Position;
    EmitVertex();

	EndPrimitive();
}