#ifdef __cplusplus
#pragma once
#define vec3 alignas(16) glm::vec3
#define vec4 alignas(16) glm::vec4
#define uint alignas(4)  uint32_t
#define bool alignas(4)  bool
#endif

#define CLUSTERS_SSBO_BINDING_INDEX                    0
#define DIRECTIONAL_LIGHTS_SSBO_BINDING_INDEX          1
#define POINT_LIGHTS_SSBO_BINDING_INDEX                2
#define SPOT_LIGHTS_SSBO_BINDING_INDEX                 3
#define POINT_LIGHTS_ELLIPSES_RADII_SSBO_BINDING_INDEX 4
#define SPOT_LIGHTS_ELLIPSES_RADII_SSBO_BINDING_INDEX  5
#define CLUSTERS_FLAGS_SSBO_BINDING_INDEX              6
#define POINT_LIGHT_INDEX_LIST_SSBO_BINDING_INDEX      7
#define SPOT_LIGHT_INDEX_LIST_SSBO_BINDING_INDEX       8
#define POINT_LIGHT_GRID_SSBO_BINDING_INDEX            9
#define SPOT_LIGHT_GRID_SSBO_BINDING_INDEX             10
#define UNIQUE_ACTIVE_CLUSTERS_SSBO_BINDING_INDEX      11
#define CULL_LIGHTS_DISPATCH_ARGS_SSBO_BINDING_INDEX   12
#define AREA_LIGHTS_SSBO_BINDING_INDEX                 13
#define AREA_LIGHT_INDEX_LIST_SSBO_BINDING_INDEX       14
#define AREA_LIGHT_GRID_SSBO_BINDING_INDEX             15

struct BaseLight
{
    vec3 color;
    float intensity;
};

struct DirectionalLight
{
    BaseLight base;
    vec3 direction;
};

struct PointLight
{
    BaseLight base;
    vec3 position;
    float radius;
};

struct SpotLight
{
    PointLight point;
    vec3 direction;
    float inner_angle;
    float outer_angle;
};

struct AreaLight
{
    BaseLight base;
    vec4 points[4];
    bool two_sided;
};

struct ClusterAABB
{
    vec4 min;
    vec4 max;
};

struct LightGrid
{
    uint offset;
    uint count;
};

#ifdef __cplusplus
#undef vec3
#undef vec4
#undef uint
#undef bool
#endif