#include "camera.h"
#include "window.h"

namespace RapidGL
{
    void Camera::update(double dt)
    {
        /* Camera Movement */
        auto movement_amount = m_move_speed * dt;

        if (Input::getKey(m_forward_key))
            move(m_position, glm::conjugate(m_orientation) * glm::vec3(0, 0, -1), movement_amount);

        if (Input::getKey(m_backward_key))
            move(m_position, glm::conjugate(m_orientation) * glm::vec3(0, 0, 1), movement_amount);

        if (Input::getKey(m_right_key))
            move(m_position, glm::conjugate(m_orientation) * glm::vec3(1, 0, 0), movement_amount);

        if (Input::getKey(m_left_key))
            move(m_position, glm::conjugate(m_orientation) * glm::vec3(-1, 0, 0), movement_amount);

        if (Input::getKey(m_up_key))
            move(m_position, glm::vec3(0, 1, 0), movement_amount);

        if (Input::getKey(m_down_key))
            move(m_position, glm::vec3(0, -1, 0), movement_amount);

        /* Camera Rotation */
        if (Input::getMouseDown(m_unlock_mouse_key))
        {
            m_free_look_locked = !m_free_look_locked;
            Input::setMouseCursorVisibility(!m_free_look_locked);

            if (m_free_look_locked)
            {
                Input::setMouseCursorPosition(Window::getCenter());
            }
        }

        if (m_free_look_locked)
        {
            auto delta_pos = Input::getMousePosition() - Window::getCenter();

            auto rot_y = delta_pos.x != 0.0f;
            auto rot_x = delta_pos.y != 0.0f;

            /* pitch */
            if (rot_x)
            {
                setOrientation(glm::angleAxis(glm::radians(delta_pos.y * m_sensitivity), glm::vec3(1, 0, 0)) * m_orientation);
            }

            /* yaw */
            if (rot_y)
            {
                setOrientation(m_orientation * glm::angleAxis(glm::radians(delta_pos.x * m_sensitivity), glm::vec3(0, 1, 0)));
            }

            if (rot_x || rot_y)
            {
                Input::setMouseCursorPosition(Window::getCenter());
            }
        }

        if (m_is_dirty)
        {
            glm::mat4 R = glm::mat4_cast(m_orientation);
            glm::mat4 T = glm::translate(glm::mat4(1.0f), -m_position);

            m_view = R * T;

            m_is_dirty = false;
        }
    }

    void Camera::move(const glm::vec3& position, const glm::vec3& dir, float amount)
    {
        setPosition(position + (dir * amount));
    }
}
