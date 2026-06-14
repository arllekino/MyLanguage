#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <algorithm>

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
    GLFWwindow* m_window = nullptr;
    std::unique_ptr<Program> m_uiProgram;
    GLuint m_vao = 0, m_vbo = 0;
    std::vector<UIVertex> m_vertices;

    float m_mouseX = 0, m_mouseY = 0;
    bool m_mouseDown = false;
    bool m_mouseDownPrev = false;
    bool m_mouseMoved = false;
    bool m_mouseJustPressed = false;

    float m_scrollDeltaY = 0.0f;

    std::string m_pendingText;
    bool m_backspacePressed = false;
    bool m_enterPressed = false;

    std::string m_focusedId;

    void SetFocused(const std::string& id)
    {
        m_focusedId = id;
    }

    void ClearFocus()
    {
        m_focusedId = "";
    }

    bool IsFocused(const std::string& id)
    {
        return m_focusedId == id;
    }

    void Init(int width, int height, const std::string& title)
    {
        if (!glfwInit())
            throw std::runtime_error("Failed to initialize GLFW");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);

        m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!m_window)
            throw std::runtime_error("Failed to create GLFW window");

        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(1);
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
            throw std::runtime_error("Failed to initialize GLAD");

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glfwSetWindowUserPointer(m_window, this);

        glfwSetCharCallback(m_window, [](GLFWwindow* w, unsigned int codepoint) {
            auto* r = static_cast<UIRenderer*>(glfwGetWindowUserPointer(w));
            if (codepoint < 128)
                r->m_pendingText += static_cast<char>(codepoint);
        });

        glfwSetKeyCallback(m_window, [](GLFWwindow* w, int key, int /*scancode*/, int action, int /*mods*/) {
            auto* r = static_cast<UIRenderer*>(glfwGetWindowUserPointer(w));
            if (action == GLFW_PRESS || action == GLFW_REPEAT) {
                if (key == GLFW_KEY_BACKSPACE) r->m_backspacePressed = true;
                if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) r->m_enterPressed = true;
            }
        });

        glfwSetScrollCallback(m_window, [](GLFWwindow* w, double /*dx*/, double dy) {
            auto* r = static_cast<UIRenderer*>(glfwGetWindowUserPointer(w));
            r->m_scrollDeltaY += static_cast<float>(dy);
        });

        glfwSetCursorPosCallback(m_window, [](GLFWwindow* w, double /*x*/, double /*y*/) {
            static_cast<UIRenderer*>(glfwGetWindowUserPointer(w))->m_mouseMoved = true;
        });

        glfwSetMouseButtonCallback(m_window, [](GLFWwindow* w, int button, int action, int /*mods*/) {
            auto* r = static_cast<UIRenderer*>(glfwGetWindowUserPointer(w));
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                if (action == GLFW_PRESS) {
                    r->m_mouseJustPressed = true;
                    r->m_mouseDown = true;
                } else if (action == GLFW_RELEASE) {
                    r->m_mouseDown = false;
                }
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

    // Returns true if any user input arrived (mouse move/click, key, scroll).
    // Clears previous frame's input state and waits for events (blocking if !alreadyDirty).
    bool WaitOrPoll(bool alreadyDirty)
    {
        m_pendingText      = "";
        m_backspacePressed = false;
        m_enterPressed     = false;
        m_scrollDeltaY     = 0.0f;
        m_mouseMoved       = false;
        m_mouseJustPressed = false;  // cleared before polling; callback sets it during poll
        m_mouseDownPrev    = m_mouseDown;

        if (alreadyDirty)
            glfwPollEvents();
        else
            glfwWaitEventsTimeout(1.0 / 60.0);

        double mx, my;
        glfwGetCursorPos(m_window, &mx, &my);
        m_mouseX = static_cast<float>(mx);
        m_mouseY = static_cast<float>(my);
        // m_mouseDown is now maintained by the mouse button callback

        return m_mouseJustPressed
            || (m_mouseDown != m_mouseDownPrev)
            || !m_pendingText.empty()
            || m_backspacePressed
            || m_enterPressed
            || m_scrollDeltaY != 0.0f;
    }

    void BeginFrame(int windowWidth, int windowHeight)
    {
        m_logicalWidth  = windowWidth;
        m_logicalHeight = windowHeight;
        m_projection = glm::ortho(0.0f, static_cast<float>(windowWidth),
                                  static_cast<float>(windowHeight), 0.0f, -1.0f, 1.0f);
        m_vertices.clear();
        m_textRenderer.BeginFrame();

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.94f, 0.95f, 0.97f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(m_uiProgram->GetId());
        GLint projLoc = glGetUniformLocation(m_uiProgram->GetId(), "uProjection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &m_projection[0][0]);
    }

    void DrawRect(float x, float y, float w, float h, glm::vec4 color)
    {
        m_vertices.push_back({{x,     y    }, color});
        m_vertices.push_back({{x + w, y    }, color});
        m_vertices.push_back({{x,     y + h}, color});

        m_vertices.push_back({{x + w, y    }, color});
        m_vertices.push_back({{x + w, y + h}, color});
        m_vertices.push_back({{x,     y + h}, color});
    }

    void DrawRoundedRect(float x, float y, float w, float h, float radius, glm::vec4 color)
    {
        if (radius <= 0.0f) { DrawRect(x, y, w, h, color); return; }
        float r = std::min(radius, std::min(w * 0.5f, h * 0.5f));

        DrawRect(x + r, y, w - 2.0f * r, h, color);
        DrawRect(x, y + r, r, h - 2.0f * r, color);
        DrawRect(x + w - r, y + r, r, h - 2.0f * r, color);

        constexpr int kSegs = 8;
        struct Corner { float cx, cy, startAngle; };
        Corner corners[4] = {
            {x + r,     y + r,     static_cast<float>(M_PI)},
            {x + w - r, y + r,     static_cast<float>(M_PI * 1.5)},
            {x + w - r, y + h - r, 0.0f},
            {x + r,     y + h - r, static_cast<float>(M_PI * 0.5)},
        };
        for (const auto& c : corners)
        {
            for (int i = 0; i < kSegs; ++i)
            {
                float a0 = c.startAngle + static_cast<float>(i)     * static_cast<float>(M_PI * 0.5 / kSegs);
                float a1 = c.startAngle + static_cast<float>(i + 1) * static_cast<float>(M_PI * 0.5 / kSegs);
                m_vertices.push_back({{c.cx,                   c.cy                  }, color});
                m_vertices.push_back({{c.cx + r * cosf(a0),   c.cy + r * sinf(a0)   }, color});
                m_vertices.push_back({{c.cx + r * cosf(a1),   c.cy + r * sinf(a1)   }, color});
            }
        }
    }

    void DrawText(float x, float y, const std::string& text, float fontSize, glm::vec4 color)
    {
        m_textRenderer.DrawText(x, y, text, fontSize, color);
    }

    float MeasureTextWidth(const std::string& text, float fontSize) const
    {
        return m_textRenderer.MeasureWidth(text, fontSize);
    }

    bool IsMouseJustPressed() const
    {
        return m_mouseJustPressed;
    }

    bool IsMouseJustReleased() const {
        return !m_mouseDown && m_mouseDownPrev;
    }

    float GetScrollDelta() const { return m_scrollDeltaY; }

    void FlushBatch()
    {
        if (m_vertices.empty()) return;
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(UIVertex), m_vertices.data());
        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));
        m_vertices.clear();
    }

    void BeginClip(float x, float y, float w, float h)
    {
        FlushBatch();
        m_textRenderer.FlushBatch(m_projection);

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
        float scaleX = static_cast<float>(fbWidth)  / static_cast<float>(m_logicalWidth);
        float scaleY = static_cast<float>(fbHeight) / static_cast<float>(m_logicalHeight);

        glEnable(GL_SCISSOR_TEST);
        glScissor(
            static_cast<int>(x * scaleX),
            static_cast<int>((m_logicalHeight - y - h) * scaleY),
            static_cast<int>(w * scaleX),
            static_cast<int>(h * scaleY)
        );
    }

    void EndClip()
    {
        FlushBatch();
        m_textRenderer.FlushBatch(m_projection);
        glDisable(GL_SCISSOR_TEST);
    }

    void EndFrame()
    {
        if (!m_vertices.empty())
        {
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0,
                            m_vertices.size() * sizeof(UIVertex), m_vertices.data());
            glBindVertexArray(m_vao);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));
        }

        m_textRenderer.EndFrame(m_projection);

        glfwSwapBuffers(m_window);
    }

    ~UIRenderer()
    {
        if (m_vao)
        {
            glDeleteVertexArrays(1, &m_vao);
        }
        if (m_vbo)
        {
            glDeleteBuffers(1, &m_vbo);
        }
        if (m_window)
        {
            glfwDestroyWindow(m_window);
        }
        glfwTerminate();
    }

private:
    TextRenderer m_textRenderer;
    glm::mat4 m_projection{1.0f};
    int m_logicalWidth = 800;
    int m_logicalHeight = 600;

    void SetupBuffers() {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, 65536 * sizeof(UIVertex), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                              reinterpret_cast<void*>(offsetof(UIVertex, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                              reinterpret_cast<void*>(offsetof(UIVertex, color)));
    }

    void InitShaders()
    {
        m_uiProgram = std::make_unique<Program>(vertexShaderCode, fragmentShaderCode);
    }
};
