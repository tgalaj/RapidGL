#pragma once

#include <map>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

namespace RapidGL
{
    class Shader final
    {
    public:
        enum class ShaderType 
        { 
            VERTEX                 = GL_VERTEX_SHADER, 
            FRAGMENT               = GL_FRAGMENT_SHADER,
            GEOMETRY               = GL_GEOMETRY_SHADER, 
            TESSELATION_CONTROL    = GL_TESS_CONTROL_SHADER,
            TESSELATION_EVALUATION = GL_TESS_EVALUATION_SHADER, 
            COMPUTE                = GL_COMPUTE_SHADER
        };

        Shader();
        explicit Shader(const std::string & compute_shader_filename);

        Shader(const std::string & vertex_shader_filename,
               const std::string & fragment_shader_filename);

        Shader(const std::string & vertex_shader_filename,
               const std::string & fragment_shader_filename,
               const std::string & geometry_shader_filename);

        Shader(const std::string & vertex_shader_filename,
               const std::string & fragment_shader_filename,
               const std::string & tessellation_control_shader_filename,
               const std::string & tessellation_evaluation_shader_filename);

        Shader(const std::string & vertex_shader_filename,
               const std::string & fragment_shader_filename,
               const std::string & geometry_shader_filename,
               const std::string & tessellation_control_shader_filename,
               const std::string & tessellation_evaluation_shader_filename);

        ~Shader();

        bool link();
        void bind() const;

        void setUniform(const std::string & uniformName, float value);
        void setUniform(const std::string & uniformName, int value);
        void setUniform(const std::string & uniformName, GLuint value);
        void setUniform(const std::string & uniformName, GLsizei count, float * value);
        void setUniform(const std::string & uniformName, GLsizei count, int * value);
        void setUniform(const std::string & uniformName, GLsizei count, glm::vec3 * vectors);
        void setUniform(const std::string & uniformName, const glm::vec2 & vector);
        void setUniform(const std::string & uniformName, const glm::vec3 & vector);
        void setUniform(const std::string & uniformName, const glm::vec4 & vector);
        void setUniform(const std::string & uniformName, const glm::mat3 & matrix);
        void setUniform(const std::string & uniformName, const glm::mat4 & matrix);
        void setUniform(const std::string & uniformName, glm::mat4 * matrices, unsigned count);

        void setSubroutine(ShaderType shader_type, const std::string& subroutine_name);

    private:
        void addAllSubroutines();

        void addShader(std::string const & file_name, GLuint type) const;
        bool getUniformLocation(const std::string & uniform_name);

        std::map<std::string, GLuint> m_subroutine_indices;
        std::map<GLenum, GLuint> m_active_subroutine_uniform_locations;

        std::map<std::string, GLint> m_uniforms_locations;

        GLuint m_program_id;
        bool m_is_linked;
    };
}
