#version 460
#include "shared.h"

layout(std430, binding = AREA_LIGHTS_SSBO_BINDING_INDEX) buffer AreaLightsSSBO
{
    AreaLight area_lights[];
};

layout(location = 0) out vec3 light_color;
layout(location = 1) flat out uint two_sided;

uniform mat4 u_view_projection;

const uint VERTICES_COUNT = 6;
const uint indices[6]     = { 0, 1, 2, 1, 3, 2};

void main()
{
    uint light_index           = gl_VertexID / VERTICES_COUNT;
    uint vertex_position_index = indices[gl_VertexID % VERTICES_COUNT];

    light_color = area_lights[light_index].base.color * area_lights[light_index].base.intensity;
    gl_Position = u_view_projection * vec4(area_lights[light_index].points[vertex_position_index].xyz, 1.0);
    two_sided   = area_lights[light_index].two_sided ? 1 : 0;
}