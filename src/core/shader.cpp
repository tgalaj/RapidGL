#include <glm/gtc/type_ptr.hpp>
#include <memory>

#include "shader.h"
#include "util.h"

namespace RGL
{
    Shader::Shader()
        : m_program_id(0),
          m_is_linked(false)
    {
        m_program_id = glCreateProgram();

        if (m_program_id == 0)
        {
            fprintf(stderr, "Error while creating program object.\n");
        }
    }

    Shader::Shader(const std::string & compute_shader_filename)
        : Shader()
    {
        addShader(compute_shader_filename, GL_COMPUTE_SHADER);
    }

    Shader::Shader(const std::string & vertex_shader_filename,
                   const std::string & fragment_shader_filename)
        : Shader()
    {
        addShader(vertex_shader_filename, GL_VERTEX_SHADER);
        addShader(fragment_shader_filename, GL_FRAGMENT_SHADER);
    }

    Shader::Shader(const std::string & vertex_shader_filename,
                   const std::string & fragment_shader_filename,
                   const std::string & geometry_shader_filename)
        : Shader(vertex_shader_filename, fragment_shader_filename)
    {
        addShader(geometry_shader_filename, GL_GEOMETRY_SHADER);
    }

    Shader::Shader(const std::string & vertex_shader_filename,
                   const std::string & fragment_shader_filename,
                   const std::string & tessellation_control_shader_filename,
                   const std::string & tessellation_evaluation_shader_filename)
        : Shader(vertex_shader_filename, fragment_shader_filename)
    {
        addShader(tessellation_control_shader_filename, GL_TESS_CONTROL_SHADER);
        addShader(tessellation_evaluation_shader_filename, GL_TESS_EVALUATION_SHADER);
    }

    Shader::Shader(const std::string & vertex_shader_filename,
                   const std::string & fragment_shader_filename,
                   const std::string & geometry_shader_filename,
                   const std::string & tessellation_control_shader_filename,
                   const std::string & tessellation_evaluation_shader_filename)
        : Shader(vertex_shader_filename,
                 fragment_shader_filename,
                 tessellation_control_shader_filename,
                 tessellation_evaluation_shader_filename)
    {
        addShader(geometry_shader_filename, GL_GEOMETRY_SHADER);
    }

    Shader::~Shader()
    {
        if (m_program_id != 0)
        {
            glDeleteProgram(m_program_id);
            m_program_id = 0;
        }
    }

    void Shader::addShader(std::string const & file_name, GLuint type) const
    {
        if (m_program_id == 0)
        {
            return;
        }

        if (file_name.empty())
        {
            fprintf(stderr, "Error: Shader's file name can't be empty.\n");

            return;
        }

        GLuint shaderObject = glCreateShader(type);

        if (shaderObject == 0)
        {
            fprintf(stderr, "Error while creating %s.\n", file_name.c_str());

            return;
        }


        std::string code = Util::LoadFile(file_name);
        code = Util::LoadShaderIncludes(code);

        const char * shader_code = code.c_str();

        glShaderSource(shaderObject, 1, &shader_code, nullptr);
        glCompileShader(shaderObject);

        GLint result;
        glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &result);

        if (result == GL_FALSE)
        {
            fprintf(stderr, "%s compilation failed!\n", file_name.c_str());

            GLint logLen;
            glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &logLen);

            if (logLen > 0)
            {
                char * log = static_cast<char *>(malloc(logLen));

                GLsizei written;
                glGetShaderInfoLog(shaderObject, logLen, &written, log);

                fprintf(stderr, "Shader log: \n%s", log);
                free(log);
            }
            getchar();
            return;
        }

        glAttachShader(m_program_id, shaderObject);
        glDeleteShader(shaderObject);
    }

    bool Shader::link()
    {
        glLinkProgram(m_program_id);

        GLint status;
        glGetProgramiv(m_program_id, GL_LINK_STATUS, &status);

        if (status == GL_FALSE)
        {
            fprintf(stderr, "Failed to link shader program!\n");

            GLint logLen;
            glGetProgramiv(m_program_id, GL_INFO_LOG_LENGTH, &logLen);

            if (logLen > 0)
            {
                char* log = (char*)malloc(logLen);
                GLsizei written;
                glGetProgramInfoLog(m_program_id, logLen, &written, log);

                fprintf(stderr, "Program log: \n%s", log);
                free(log);
            }
        }
        else
        {
            m_is_linked = true;

            addAllSubroutines();
        }

        return m_is_linked;
    }

    void Shader::setTransformFeedbackVaryings(const std::vector<const char*>& output_names, GLenum buffer_mode) const
    {
        glTransformFeedbackVaryings(m_program_id, output_names.size(), output_names.data(), buffer_mode);
    }

    void Shader::bind() const
    {
        if (m_program_id != 0 && m_is_linked)
        {
            glUseProgram(m_program_id);
        }
    }

    bool Shader::getUniformLocation(const std::string & uniform_name)
    {
        GLint uniform_location = glGetUniformLocation(m_program_id, uniform_name.c_str());

        if (uniform_location != -1)
        {
            m_uniforms_locations[uniform_name] = uniform_location;
            return true;
        }

        return false;
    }

    void Shader::setUniform(const std::string & uniformName, float value)
    {
        if (m_uniforms_locations.count(uniformName))
        {
            glProgramUniform1f(m_program_id, m_uniforms_locations[uniformName], value);
        }
        else
        {
            if (getUniformLocation(uniformName))
            {
                glProgramUniform1f(m_program_id, m_uniforms_locations[uniformName], value);
            }
        }
    }

    void Shader::setUniform(const std::string & uniformName, int value)
    {
        if (m_uniforms_locations.count(uniformName))
        {
            glProgramUniform1i(m_program_id, m_uniforms_locations[uniformName], value);
        }
        else
        {
            if (getUniformLocation(uniformName))
            {
                glProgramUniform1i(m_program_id, m_uniforms_locations[uniformName], value);
            }
        }
    }

    void Shader::setUniform(const std::string & uniformName, GLuint value)
    {
        if (m_uniforms_locations.count(uniformName))
        {
            glProgramUniform1ui(m_program_id, m_uniforms_locations.at(uniformName), value);
        }
        else
        {
            if (getUniformLocation(uniformName))
            {
                glProgramUniform1ui(m_program_id, m_uniforms_locations[uniformName], value);
            }
        }
    }

    void Shader::setUniform(const std::string & uniformName, GLsizei count, float * value)
    {
        if (m_uniforms_locations.count(uniformName))
        {
            glProgramUniform1fv(m_program_id, m_uniforms_locations[uniformName], count, value);
        }
        else
        {
            if (getUniformLocation(uniformName))
            {
                glProgramUniform1fv(m_program_id, m_uniforms_locations[uniformName], count, value);
            }
        }
    }

    void Shader::setUniform(const std::string & uniformName, GLsizei count, int * value)
    {
        if (m_uniforms_locations.count(uniformName))
        {
            glProgramUniform1iv(m_program_id, m_uniforms_locations[uniformName], count, value);
        }
        else
        {
            if (getUniformLocation(uniformName))
            {
                glProgramUniform1iv(m_program_id, m_uniforms_locations[uniformName], count, value);
            }
        }
    }

    void Shader::setUniform(const std::string & uniformName, GLsizei count, glm::vec3 * vectors)
    {
        if (m_uniforms_locations.count(uniformName))
        {
            glProgramUniform3fv(m_program_id, m_uniforms_locations[uniformName], count, glm::value_ptr(vectors[0]));
        }
        else
        {
            if (getUniformLocation(uniformName))
            {
                glProgramUniform3fv(m_program_id, m_uniforms_locations[uniformName], count, glm::value_ptr(vectors[0]));
            }
        }
    }

    void Shader::setUniform(const std::string & uniformName, const glm::vec2 & vector)
    {
        if (m_uniforms_locations.count(uniformName))
        {
            glProgramUniform2fv(m_program_id, m_uniforms_locations[uniformName], 1, glm::value_ptr(vector));
        }
        else
        {
            if (getUniformLocation(uniformName))
            {
                glProgramUniform2fv(m_program_id, m_uniforms_locations[uniformName], 1, glm::value_ptr(vector));
            }
        }
    }

    void Shader::setUniform(const std::string & uniformName, const glm::vec3 & vector)
    {
        if (m_uniforms_locations.count(uniformName))
        {
            glProgramUniform3fv(m_program_id, m_uniforms_locations[uniformName], 1, glm::value_ptr(vector));
        }
        else
        {
            if (getUniformLocation(uniformName))
            {
                glProgramUniform3fv(m_program_id, m_uniforms_locations[uniformName], 1, glm::value_ptr(vector));
            }
        }
    }

    void Shader::setUniform(const std::string & uniformName, const glm::vec4 & vector)
    {
        if (m_uniforms_locations.count(uniformName))
        {
            glProgramUniform4fv(m_program_id, m_uniforms_locations[uniformName], 1, glm::value_ptr(vector));
        }
        else
        {
            if (getUniformLocation(uniformName))
            {
                glProgramUniform4fv(m_program_id, m_uniforms_locations[uniformName], 1, glm::value_ptr(vector));
            }
        }
    }

    void Shader::setUniform(const std::string & uniformName, const glm::mat3 & matrix)
    {
        if (m_uniforms_locations.count(uniformName))
        {
            glProgramUniformMatrix3fv(m_program_id, m_uniforms_locations[uniformName], 1, GL_FALSE, glm::value_ptr(matrix));
        }
        else
        {
            if (getUniformLocation(uniformName))
            {
                glProgramUniformMatrix3fv(m_program_id, m_uniforms_locations[uniformName], 1, GL_FALSE, glm::value_ptr(matrix));
            }
        }
    }

    void Shader::setUniform(const std::string & uniformName, const glm::mat4 & matrix)
    {
        if (m_uniforms_locations.count(uniformName))
        {
            glProgramUniformMatrix4fv(m_program_id, m_uniforms_locations[uniformName], 1, GL_FALSE, glm::value_ptr(matrix));
        }
        else
        {
            if (getUniformLocation(uniformName))
            {
                glProgramUniformMatrix4fv(m_program_id, m_uniforms_locations[uniformName], 1, GL_FALSE, glm::value_ptr(matrix));
            }
        }
    }

    void Shader::setUniform(const std::string& uniformName, float* values, unsigned count)
    {
        if (m_uniforms_locations.count(uniformName))
        {
            glProgramUniform1fv(m_program_id, m_uniforms_locations[uniformName], count, &values[0]);
        }
        else
        {
            if (getUniformLocation(uniformName))
            {
                glProgramUniform1fv(m_program_id, m_uniforms_locations[uniformName], count, &values[0]);
            }
        }
    }

    void Shader::setUniform(const std::string& uniformName, glm::vec2* values, unsigned count)
    {
        if (m_uniforms_locations.count(uniformName))
        {
            glProgramUniform2fv(m_program_id, m_uniforms_locations[uniformName], count, &values[0][0]);
        }
        else
        {
            if (getUniformLocation(uniformName))
            {
                glProgramUniform2fv(m_program_id, m_uniforms_locations[uniformName], count, &values[0][0]);
            }
        }
    }

    void Shader::setUniform(const std::string & uniformName, glm::mat4 * matrices, unsigned count)
    {
        if (m_uniforms_locations.count(uniformName))
        {
            glProgramUniformMatrix4fv(m_program_id, m_uniforms_locations[uniformName], count, GL_FALSE, &matrices[0][0][0]);
        }
        else
        {
            if (getUniformLocation(uniformName))
            {
                glProgramUniformMatrix4fv(m_program_id, m_uniforms_locations[uniformName], count, GL_FALSE, &matrices[0][0][0]);
            }
        }
    }

    void Shader::setUniform(const std::string& uniformName, glm::mat2x4* matrices, unsigned count)
    {
        if (m_uniforms_locations.count(uniformName))
        {
            glProgramUniformMatrix2x4fv(m_program_id, m_uniforms_locations[uniformName], count, GL_FALSE, &matrices[0][0][0]);
        }
        else
        {
            if (getUniformLocation(uniformName))
            {
                glProgramUniformMatrix2x4fv(m_program_id, m_uniforms_locations[uniformName], count, GL_FALSE, &matrices[0][0][0]);
            }
        }
    }

    void Shader::setSubroutine(ShaderType shader_type, const std::string & subroutine_name)
    {
        glUniformSubroutinesuiv(GLenum(shader_type), m_active_subroutine_uniform_locations[GLenum(shader_type)], &m_subroutine_indices[subroutine_name]);
    }

    void Shader::addAllSubroutines()
    {
        GLenum interfaces[]    = { GL_VERTEX_SUBROUTINE, GL_FRAGMENT_SUBROUTINE };
        GLenum shader_stages[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };

        GLint interfaces_count = std::size(interfaces);

        for(GLint i = 0; i < interfaces_count; ++i)
        {
            /* Get all active subroutines */
            GLenum program_interface = interfaces[i];

            GLint num_subroutines = 0;
            glGetProgramInterfaceiv(m_program_id, program_interface, GL_ACTIVE_RESOURCES, &num_subroutines);

            const GLenum properties[] = { GL_NAME_LENGTH };
            const GLint properties_size = sizeof(properties) / sizeof(properties[0]);

            GLint count_subroutine_locations = 0;
            glGetProgramStageiv(m_program_id, shader_stages[i], GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &count_subroutine_locations);
            m_active_subroutine_uniform_locations[shader_stages[i]] = count_subroutine_locations;

            for (GLint j = 0; j < num_subroutines; ++j)
            {
                GLint values[properties_size];
                GLint length = 0;
                glGetProgramResourceiv(m_program_id, program_interface, j, properties_size, properties, properties_size, &length, values);

                std::vector<char> name_data(values[0]);
                glGetProgramResourceName(m_program_id, program_interface, j, name_data.size(), nullptr, &name_data[0]);
                std::string subroutine_name(name_data.begin(), name_data.end() - 1);

                GLuint subroutine_index = glGetSubroutineIndex(m_program_id, shader_stages[i], subroutine_name.c_str());

                m_subroutine_indices[subroutine_name] = subroutine_index;
            }
        }
    }
}
