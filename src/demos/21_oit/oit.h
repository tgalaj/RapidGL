#pragma once
#include "core_app.h"

#include "camera.h"
#include "static_model.h"
#include "shader.h"

#include <memory>
#include <vector>

class OIT : public RGL::CoreApp
{
public:
    struct ListNode
    {
        glm::vec4 color;
        float depth;
        int coverage;
        uint32_t next;
    };

    OIT();
    ~OIT();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()              override;

private:
    std::shared_ptr<RGL::Camera> m_camera;
    std::shared_ptr<RGL::Shader> m_oit_linked_list_shader, m_oit_render_shader;

    RGL::StaticModel m_sphere_model;
    std::vector<glm::mat4> m_objects_model_matrices;
    std::vector<glm::vec3> m_objects_colors;

    RGL::StaticModel m_dragon_model;
    glm::mat4 m_dragon_model_matrix;
    glm::vec3 m_dragon_color;

    std::vector<uint32_t> m_head_pointers_clear_data;
    GLuint m_fsq_vao;
    GLuint m_linked_lists_buffer, m_list_info_buffer;
    GLuint m_head_pointers_image2d;

    uint32_t m_max_nodes;
    glm::vec3 m_grid_dimensions;
    float m_transparency;

    uint32_t m_current_model = 0;
    std::vector<std::string> m_models_names_combo_box = { "spheres", "dragon"};
};