#pragma once
#include <cassert>
#include <stdexcept>
#include <string>
#include <GLFW/glfw3.h>

class Shader
{
public:
    explicit Shader(const GLenum type)
        : m_shader(glCreateShader(type))
    {
        if (!m_shader)
        {
            throw std::runtime_error("Failed to create shader");
        }
    }

    Shader(Shader&& other) noexcept
        : m_shader(std::exchange(other.m_shader, 0))
    {
    }

    Shader& operator=(Shader&& other) noexcept
    {
        std::swap(m_shader, other.m_shader);
        return *this;
    }

    ~Shader()
    {
        glDeleteShader(m_shader);
    }

    void SetSoucre(const char* text) noexcept
    {
        assert(text);
        assert(m_shader);
        glShaderSource(m_shader, 1, &text, nullptr);
    }

    void Compile() noexcept
    {
        assert(m_shader);
        glCompileShader(m_shader);
    }

    void GetParameter(GLenum paramName, GLint* param) const noexcept
    {
        glGetShaderiv(m_shader, paramName, param);
    }

    std::string GetInfoLog()
    {
        int infoLength = 0;
        GetParameter(GL_INFO_LOG_LENGTH, &infoLength);
        std::string log(static_cast<size_t>(infoLength), ' ');
        GLsizei actualSize = 0;
        glGetShaderInfoLog(m_shader, infoLength, &actualSize, log.data());
        log.resize(static_cast<size_t>(actualSize));
        return log;
    }

    bool IsCompiled() const noexcept
    {
        int compileStatus = GL_FALSE;
        GetParameter(GL_COMPILE_STATUS, &compileStatus);
        return compileStatus == GL_TRUE;
    }

    operator GLuint() const noexcept
    {
        return m_shader;
    }
private:
    GLuint m_shader = 0u;
};