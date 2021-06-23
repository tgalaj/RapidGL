#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace RGL
{
    constexpr static uint32_t INVALID_MATERIAL = 0xffffffff;

    struct Vertex
    {
        Vertex() = default;

        glm::vec3 m_position;
        glm::vec3 m_normal;
        glm::vec2 m_texcoord;
        glm::vec3 m_tangent;
    };

    class MeshPart final
    {
    public:
        MeshPart()
            : m_base_vertex   (0),
              m_base_index    (0),
              m_material_index(INVALID_MATERIAL),
              m_indices_count (0) { }

    private:
        uint32_t m_base_vertex;
        uint32_t m_base_index;
        uint32_t m_material_index;
        uint32_t m_indices_count;

        friend class StaticModel;
    };
}
