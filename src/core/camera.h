#pragma once

#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "input.h"

namespace RGL
{
    class Camera
    {
    public:
        /*
         * Perspective camera
         */
        Camera(float fov,
               float aspect_ratio,
               float z_near,
               float z_far)
               : Camera()
        {
            m_projection   = glm::perspective(glm::radians(fov), aspect_ratio, z_near, z_far);
            m_aspect_ratio = aspect_ratio;
            m_near         = z_near;
            m_far          = z_far;
            m_fov          = fov;
        }

        /*
         * Ortho camera
         */
        Camera(float left,
               float right,
               float bottom,
               float top,
               float z_near,
               float z_far)
               : Camera(true)
        {
            m_projection   = glm::ortho(left, right, bottom, top, z_near, z_far);
            m_aspect_ratio = 1.0f;
            m_near         = z_near;
            m_far          = z_far;
            m_fov          = 1.0f;
        }

        void setPosition(float x, float y, float z)
        {
            m_position = glm::vec3(x, y, z);
            m_is_dirty = true;
        }

        void setPosition(const glm::vec3& position)
        {
            m_position = position;
            m_is_dirty = true;
        }

        /*
         * Set orientation using Euler Angles in degrees
         */
        void setOrientation(float x, float y, float z)
        {
            m_orientation = glm::angleAxis(glm::radians(x), glm::vec3(1.0f, 0.0f, 0.0f)) *
                            glm::angleAxis(glm::radians(y), glm::vec3(0.0f, 1.0f, 0.0f)) *
                            glm::angleAxis(glm::radians(z), glm::vec3(0.0f, 0.0f, 1.0f));
            
            m_direction = glm::normalize(glm::conjugate(m_orientation) * glm::vec3(0.0f, 0.0f, 1.0f));
            m_is_dirty  = true;
        }

        /*
         * Set orientation using explicit direction vector
         */
        void setOrientation(const glm::vec3 & direction)
        {
            m_orientation = glm::quatLookAt(glm::normalize(direction), glm::vec3(0.0, 1.0, 0.0));
            m_direction = glm::normalize(glm::conjugate(m_orientation) * glm::vec3(0.0f, 0.0f, 1.0f));
            m_is_dirty  = true;
        }

        /*
        * Set orientation using axis and angle in degrees
        */
        void setOrientation(const glm::vec3& axis, float angle)
        {
            m_orientation = glm::angleAxis(glm::radians(angle), glm::normalize(axis));
            m_direction   = glm::normalize(glm::conjugate(m_orientation) * glm::vec3(0.0f, 0.0f, 1.0f));
            m_is_dirty    = true;
        }

        void setOrientation(const glm::quat& quat)
        {
            m_orientation = quat;
            m_direction   = glm::normalize(glm::conjugate(m_orientation) * glm::vec3(0.0f, 0.0f, 1.0f));
            m_is_dirty    = true;
        }

        glm::quat orientation() const { return m_orientation; }
        glm::vec3 position()    const { return m_position; }
        glm::vec3 direction()   const { return m_direction; }
        float AspectRatio()     const { return m_aspect_ratio; }
        float NearPlane()       const { return m_near; }
        float FarPlane()        const { return m_far; }
        float FOV()             const { return m_fov; }

        void update(double dt);

        glm::mat4 m_view;
        glm::mat4 m_projection;
        const bool m_is_ortho;

        float m_sensitivity;
        float m_move_speed;

        KeyCode m_unlock_mouse_key;
        KeyCode m_forward_key;
        KeyCode m_backward_key;
        KeyCode m_left_key;
        KeyCode m_right_key;
        KeyCode m_up_key;
        KeyCode m_down_key;
    
    private:
        Camera(bool is_ortho = false) 
            : m_orientation     (glm::vec3(0.0f)),
              m_position        (glm::vec3(0.0f)),
              m_direction       (glm::vec3(0.0f, 0.0f, -1.0f)),
              m_near            (0.01f),
              m_far             (100.0f),
              m_aspect_ratio    (1.0f),
              m_fov             (60.0f),
              m_is_dirty        (true),
              m_is_mouse_move   (false),
              m_view            (1.0f),
              m_projection      (1.0f),
              m_is_ortho        (is_ortho),
              m_sensitivity     (0.2),
              m_move_speed      (10.0),
              m_unlock_mouse_key(KeyCode::MouseRight),
              m_forward_key     (KeyCode::W),
              m_backward_key    (KeyCode::S),
              m_left_key        (KeyCode::A),
              m_right_key       (KeyCode::D),
              m_up_key          (KeyCode::E),
              m_down_key        (KeyCode::Q) {}

        glm::quat m_orientation;
        glm::vec3 m_position;
        glm::vec3 m_direction;
        float m_near;
        float m_far;
        float m_aspect_ratio;
        float m_fov; // in degrees


        glm::vec2 m_mouse_pressed_position;
        bool      m_is_dirty;
        bool      m_is_mouse_move;

        void move(const glm::vec3 & position, const glm::vec3& dir, float amount);
    };
}
