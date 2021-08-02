// Based on https://github.com/keijiro/StandardGeometryShader

#version 460 core
#define PI 3.141592653589793238462643

layout(triangles) in;
layout(triangle_strip, max_vertices = 15) out;

layout (location = 0) in vec2 in_texcoord[];
layout (location = 1) in vec3 in_world_pos[];
layout (location = 2) in vec3 in_normal[];

layout (location = 0) out vec2 out_texcoord;
layout (location = 1) out vec3 out_world_pos;
layout (location = 2) out vec3 out_normal;

uniform float u_time;
uniform float u_extrusion_amount;
uniform mat4 u_view_projection;

vec3 constructNormal(vec3 v1, vec3 v2, vec3 v3)
{
    return normalize(cross(v2 - v1, v3 - v1));
}

void outputVertex(vec3 world_pos, vec3 world_normal, vec2 uv)
{
    out_world_pos = world_pos;
    out_normal    = world_normal;
    out_texcoord  = uv;
    gl_Position   = u_view_projection * vec4(world_pos, 1.0);
    EmitVertex();
}

void main()
{
    vec3 wp0 = in_world_pos[0];
    vec3 wp1 = in_world_pos[1];
    vec3 wp2 = in_world_pos[2];

    vec2 uv0 = in_texcoord[0];
    vec2 uv1 = in_texcoord[1];
    vec2 uv2 = in_texcoord[2];

    // Extrusion amount
    float ext  = clamp(0.4 - cos(u_time * PI * 2) * 0.41, 0, 1);
          ext *= 1 + 0.3 * sin(gl_PrimitiveIDIn  * 832.37843 + u_time * 88.76);
    // Extrusion points
    vec3 offs = constructNormal(wp0, wp1, wp2) * ext * u_extrusion_amount;
    vec3 wp3 = wp0 + offs;
    vec3 wp4 = wp1 + offs;
    vec3 wp5 = wp2 + offs;

    // Cap triangle
    vec3 wn = constructNormal(wp3, wp4, wp5);
    float np = clamp(ext, 0, 1);
    vec3 wn0 = mix(in_normal[0], wn, np);
    vec3 wn1 = mix(in_normal[1], wn, np);
    vec3 wn2 = mix(in_normal[2], wn, np);

    outputVertex(wp3, wn0, uv0);
    outputVertex(wp4, wn1, uv1);
    outputVertex(wp5, wn2, uv2);

    // Side faces
    wn = constructNormal(wp4, wp1, wp2);
    outputVertex(wp5, wn, uv2);
    outputVertex(wp2, wn, uv2);
    outputVertex(wp4, wn, uv1);
    outputVertex(wp1, wn, uv1);

    wn = constructNormal(wp4, wp3, wp0);
    outputVertex(wp4, wn, uv1);
    outputVertex(wp1, wn, uv1);
    outputVertex(wp3, wn, uv0);
    outputVertex(wp0, wn, uv0);
    
    wn = constructNormal(wp3, wp5, wp2);
    outputVertex(wp3, wn, uv0);
    outputVertex(wp0, wn, uv0);
    outputVertex(wp5, wn, uv2);
    outputVertex(wp2, wn, uv2);

	EndPrimitive();
}