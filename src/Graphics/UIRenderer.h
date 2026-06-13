#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <memory>

#include "./Utils/Shader/Program.h"
#include "./Utils/TextRenderer.h"

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

    // Mouse state
    float mouseX = 0, mouseY = 0;
    bool  mouseDown = false;
    bool  mouseDownPrev = false;

    // Keyboard state — characters typed this frame
    std::string pendingText;
    bool backspacePressed = false;
    bool enterPressed = false;

    // Focus state
    std::string m_focusedId;

    void SetFocused(const std::string& id) { m_focusedId = id; }
    void ClearFocus() { m_focusedId = ""; }
    bool IsFocused(const std::string& id) { return m_focusedId == id; }

    void Init(int width, int height, const std::string& title)
    {
        if (!glfwInit())
            throw std::runtime_error("Failed to initialize GLFW");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);

        window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!window)
            throw std::runtime_error("Failed to create GLFW window");

        glfwMakeContextCurrent(window);
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
            throw std::runtime_error("Failed to initialize GLAD");

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Register input callbacks
        glfwSetWindowUserPointer(window, this);

        glfwSetCharCallback(window, [](GLFWwindow* w, unsigned int codepoint) {
            auto* r = static_cast<UIRenderer*>(glfwGetWindowUserPointer(w));
            if (codepoint < 128)
                r->pendingText += static_cast<char>(codepoint);
        });

        glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int /*scancode*/, int action, int /*mods*/) {
            auto* r = static_cast<UIRenderer*>(glfwGetWindowUserPointer(w));
            if (action == GLFW_PRESS || action == GLFW_REPEAT) {
                if (key == GLFW_KEY_BACKSPACE) r->backspacePressed = true;
                if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) r->enterPressed = true;
            }
        });

        SetupBuffers();
        InitShaders();
        m_textRenderer.Init();

        m_projection = glm::ortho(0.0f, static_cast<float>(width),
                                  static_cast<float>(height), 0.0f, -1.0f, 1.0f);
        m_logicalWidth  = width;
        m_logicalHeight = height;
    }

    void BeginFrame(int windowWidth, int windowHeight)
    {
        m_logicalWidth  = windowWidth;
        m_logicalHeight = windowHeight;
        m_projection = glm::ortho(0.0f, static_cast<float>(windowWidth),
                                  static_cast<float>(windowHeight), 0.0f, -1.0f, 1.0f);
        vertices.clear();
        m_textRenderer.BeginFrame();

        // Poll mouse
        mouseDownPrev = mouseDown;
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        mouseX    = static_cast<float>(mx);
        mouseY    = static_cast<float>(my);
        mouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        // Reset per-frame keyboard flags
        pendingText      = "";
        backspacePressed = false;
        enterPressed     = false;

        glfwPollEvents();

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.1f, 0.1f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(uiProgram->GetId());
        GLint projLoc = glGetUniformLocation(uiProgram->GetId(), "uProjection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &m_projection[0][0]);
    }

    void DrawRect(float x, float y, float w, float h, glm::vec4 color)
    {
        vertices.push_back({{x,     y    }, color});
        vertices.push_back({{x + w, y    }, color});
        vertices.push_back({{x,     y + h}, color});

        vertices.push_back({{x + w, y    }, color});
        vertices.push_back({{x + w, y + h}, color});
        vertices.push_back({{x,     y + h}, color});
    }

    void DrawText(float x, float y, const std::string& text, float fontSize, glm::vec4 color)
    {
        m_textRenderer.DrawText(x, y, text, fontSize, color);
    }

    float MeasureTextWidth(const std::string& text, float fontSize) const
    {
        return m_textRenderer.MeasureWidth(text, fontSize);
    }

    bool IsMouseJustPressed()  const { return mouseDown && !mouseDownPrev; }
    bool IsMouseJustReleased() const { return !mouseDown && mouseDownPrev; }

    void EndFrame()
    {
        // 1. Flush colored rects
        if (!vertices.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0,
                            vertices.size() * sizeof(UIVertex), vertices.data());
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
        }

        // 2. Flush text quads
        m_textRenderer.EndFrame(m_projection);

        glfwSwapBuffers(window);
    }

    ~UIRenderer()
    {
        if (vao) glDeleteVertexArrays(1, &vao);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (window) glfwDestroyWindow(window);
        glfwTerminate();
    }

private:
    TextRenderer m_textRenderer;
    glm::mat4    m_projection{1.0f};
    int          m_logicalWidth  = 800;
    int          m_logicalHeight = 600;

    void SetupBuffers() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 12000 * sizeof(UIVertex), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                              reinterpret_cast<void*>(offsetof(UIVertex, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                              reinterpret_cast<void*>(offsetof(UIVertex, color)));
    }

    void InitShaders()
    {
        uiProgram = std::make_unique<Program>(vertexShaderCode, fragmentShaderCode);
    }
};
