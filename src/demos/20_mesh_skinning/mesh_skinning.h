#pragma once
#include "core_app.h"

#include "camera.h"
#include "animated_model.h"
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
    std::shared_ptr<RGL::Shader> m_simple_skinning_shader;

    RGL::AnimatedModel m_animated_model;
    std::vector<std::string> m_animations_names;
    std::vector<glm::mat4> m_bone_transforms;
    glm::mat4 m_object_model_matrix;;

    uint32_t m_current_animation_index;
    float m_animation_speed;
    float m_gamma;
};