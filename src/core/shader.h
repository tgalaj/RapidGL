#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

namespace RGL
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
        explicit Shader(const std::filesystem::path & compute_shader_filepath);

        Shader(const std::filesystem::path & vertex_shader_filepath,
               const std::filesystem::path & fragment_shader_filepath);

        Shader(const std::filesystem::path & vertex_shader_filepath,
               const std::filesystem::path & fragment_shader_filepath,
               const std::filesystem::path & geometry_shader_filepath);

        Shader(const std::filesystem::path & vertex_shader_filepath,
               const std::filesystem::path & fragment_shader_filepath,
               const std::filesystem::path & tessellation_control_shader_filepath,
               const std::filesystem::path & tessellation_evaluation_shader_filepath);

        Shader(const std::filesystem::path & vertex_shader_filepath,
               const std::filesystem::path & fragment_shader_filepath,
               const std::filesystem::path & geometry_shader_filepath,
               const std::filesystem::path & tessellation_control_shader_filepath,
               const std::filesystem::path & tessellation_evaluation_shader_filepath);

        ~Shader();

        bool link();
        void setTransformFeedbackVaryings(const std::vector<const char*>& output_names, GLenum buffer_mode) const;
        void bind() const;

        void setUniform(const std::string & uniform_name, float value);
        void setUniform(const std::string & uniform_name, int value);
        void setUniform(const std::string & uniform_name, GLuint value);
        void setUniform(const std::string & uniform_name, GLsizei count, float * value);
        void setUniform(const std::string & uniform_name, GLsizei count, int * value);
        void setUniform(const std::string & uniform_name, GLsizei count, glm::vec3 * vectors);
        void setUniform(const std::string & uniform_name, const glm::vec2 & vector);
        void setUniform(const std::string & uniform_name, const glm::vec3 & vector);
        void setUniform(const std::string & uniform_name, const glm::vec4 & vector);
        void setUniform(const std::string & uniform_name, const glm::uvec3 & vector);
        void setUniform(const std::string & uniform_name, const glm::mat3 & matrix);
        void setUniform(const std::string & uniform_name, const glm::mat4 & matrix);
        void setUniform(const std::string & uniform_name, float* values, unsigned count);
        void setUniform(const std::string & uniform_name, glm::vec2* values, unsigned count);
        void setUniform(const std::string & uniform_name, glm::mat4 * matrices, unsigned count);
        void setUniform(const std::string & uniform_name, glm::mat2x4 * matrices, unsigned count);

        void setSubroutine(ShaderType shader_type, const std::string& subroutine_name);

    private:
        void addAllSubroutines();

        void addShader(const std::filesystem::path & filepath, GLuint type) const;
        bool getUniformLocation(const std::string & uniform_name);

        std::map<std::string, GLuint> m_subroutine_indices;
        std::map<GLenum, GLuint> m_active_subroutine_uniform_locations;

        std::map<std::string, GLint> m_uniforms_locations;

        GLuint m_program_id;
        bool m_is_linked;
    };
}
