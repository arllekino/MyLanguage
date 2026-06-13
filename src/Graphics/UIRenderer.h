#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <memory>

#include "./Utils/Shader/Program.h"

inline const char* vertexShaderCode = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec4 aColor;
    out vec4 oColor;
    uniform mat4 uProjection;

    void main()
    {
        gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
        oColor = aColor;
    }
)";

inline const char* fragmentShaderCode = R"(
    #version 330 core
    in vec4 oColor;
    out vec4 FragColor;

    void main()
    {
        FragColor = oColor;
    }
)";

struct UIVertex
{
    glm::vec2 position;
    glm::vec4 color;
};

class UIRenderer
{
public:
    GLFWwindow* window = nullptr;
    std::unique_ptr<Program> uiProgram;
    GLuint vao = 0, vbo = 0;
    std::vector<UIVertex> vertices;

    void Init(int width, int height, const std::string& title)
    {
        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }
        
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!window)
        {
            throw std::runtime_error("Failed to create GLFW window");
        }
        
        glfwMakeContextCurrent(window);
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        {
            throw std::runtime_error("Failed to initialize GLAD");
        }
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        SetupBuffers();
        InitShaders();
    }

    void BeginFrame(int windowWidth, int windowHeight)
    {
        vertices.clear();
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.1f, 0.1f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(uiProgram->GetId());

        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight), 0.0f, -1.0f, 1.0f);
        GLint projLoc = glGetUniformLocation(uiProgram->GetId(), "uProjection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    }

    void DrawRect(float x, float y, float w, float h, glm::vec4 color)
    {
        vertices.push_back({{x, y}, color});
        vertices.push_back({{x + w, y}, color});
        vertices.push_back({{x, y + h}, color});

        vertices.push_back({{x + w, y}, color});
        vertices.push_back({{x + w, y + h}, color});
        vertices.push_back({{x, y + h}, color});
    }

    void EndFrame() const
    {
        if (vertices.empty()) return;

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(UIVertex), vertices.data());

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ~UIRenderer()
    {
        if (vao) glDeleteVertexArrays(1, &vao);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (window) glfwDestroyWindow(window);
        glfwTerminate();
    }

private:
    void SetupBuffers() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        
        glBufferData(GL_ARRAY_BUFFER, 12000 * sizeof(UIVertex), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), reinterpret_cast<void *>(offsetof(UIVertex, position)));
        
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex), reinterpret_cast<void *>(offsetof(UIVertex, color)));
    }

    void InitShaders()
    {
        uiProgram = std::make_unique<Program>(vertexShaderCode, fragmentShaderCode);
    }
};