#pragma once
#include "core_app.h"

#include "camera.h"
#include "static_model.h"
#include "shader.h"

#include <memory>
#include <vector>

class MeshSkinning : public RGL::CoreApp
{
public:
    MeshSkinning();
    ~MeshSkinning();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()               override;

private:
    std::shared_ptr<RGL::Camera> m_camera;
    std::shared_ptr<RGL::Shader> m_simple_texturing_shader;
    
    RGL::TextureSampler m_sampler;

    std::vector<RGL::StaticModel> m_objects;
    std::vector<glm::mat4> m_objects_model_matrices;
    std::vector<glm::vec3> m_objects_colors;

    float m_mix_factor;
};