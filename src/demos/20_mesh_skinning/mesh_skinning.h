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
    void render_gui()              override;

private:
    enum class SkinningMethod { LBS, DQS };
    const std::string m_skinning_methods_names[2] = { "Linear Blend Skinning", "Dual Quaternion Blend Skinning" };

    std::shared_ptr<RGL::Camera> m_camera;
    std::shared_ptr<RGL::Shader> m_lbs_skinning_shader, m_dqs_skinning_shader;

    RGL::AnimatedModel m_animated_model;
    std::vector<std::string> m_animations_names;
    glm::mat4 m_object_model_matrix;;

    std::vector<glm::mat4> m_bone_transforms;
    std::vector<glm::mat2x4> m_bone_transforms_dq;

    SkinningMethod m_skinning_method;
    uint32_t m_current_animation_index;
    float m_animation_speed;
    float m_gamma;
};