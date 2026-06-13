#pragma once
#include <cassert>
#include <stdexcept>
#include <string>
#include <utility>
#include <GLFW/glfw3.h>

class ShaderProgram
{
public:
    ShaderProgram()
        : m_program(glCreateProgram())
    {
        if (!m_program)
        {
            throw std::runtime_error("Failed to create shader program");
        }
    }

    ShaderProgram(ShaderProgram&& other) noexcept
        : m_program(std::exchange(other.m_program, 0))
    {
    }

    ~ShaderProgram()
    {
        glDeleteProgram(m_program);
    }

    ShaderProgram& operator=(ShaderProgram&& other) noexcept
    {
        std::swap(m_program, other.m_program);
        return *this;
    }

    void AttachShader(GLuint shader) noexcept
    {
        assert(shader);
        assert(m_program);
        glAttachShader(m_program, shader);
    }

    void Link() noexcept
    {
        assert(m_program);
        glLinkProgram(m_program);
    }

    void Validate() noexcept
    {
        assert(m_program);
        glValidateProgram(m_program);
    }

    bool IsValid() const noexcept
    {
        GLint status = GL_FALSE;
        GetParameter(GL_VALIDATE_STATUS, &status);
        return status == GL_TRUE;
    }

    bool IsLinked() const noexcept
    {
        GLint status = GL_FALSE;
        GetParameter(GL_LINK_STATUS, &status);
        return status == GL_TRUE;
    }

    void GetParameter(GLenum paramName, GLint* param) const noexcept
    {
        assert(m_program);
        assert(param);
        glGetProgramiv(m_program, paramName, param);
    }

    std::string GetInfoLog() const noexcept
    {
        GLint bufSize = 0;
        GetParameter(GL_INFO_LOG_LENGTH, &bufSize);
        std::string log(static_cast<size_t>(bufSize), ' ');

        GLsizei actualLength = 0;
        glGetProgramInfoLog(m_program, bufSize, &actualLength, log.data());
        log.resize(static_cast<size_t>(actualLength));
        return log;
    }

    [[nodiscard]] GLint GetUniformLocation(const char* name) const
    {
        return glGetUniformLocation(m_program, name);
    }

    GLuint GetId() const noexcept
    {
        return m_program;
    }
private:
    GLuint m_program;
};
