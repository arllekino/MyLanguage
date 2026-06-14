#pragma once

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <fstream>

struct TextVertex {
    glm::vec2 position;
    glm::vec2 texCoord;
};

class TextRenderer {
public:
    static constexpr float BAKE_SIZE = 48.0f;
    static constexpr int ATLAS_W = 512, ATLAS_H = 512;

    bool Init() {
        static const char* fontPaths[] = {
            "/System/Library/Fonts/SFNS.ttf",
            "/System/Library/Fonts/Helvetica.ttc",
            "/System/Library/Fonts/Geneva.ttf",
            "/Library/Fonts/Arial Unicode.ttf",
        };

        std::vector<uint8_t> fontData;
        for (auto* path : fontPaths) {
            std::ifstream f(path, std::ios::binary | std::ios::ate);
            if (!f) continue;
            auto sz = f.tellg(); f.seekg(0);
            fontData.resize(sz);
            f.read(reinterpret_cast<char*>(fontData.data()), sz);
            break;
        }
        if (fontData.empty()) return false;

        int fontOffset = stbtt_GetFontOffsetForIndex(fontData.data(), 0);
        if (fontOffset < 0) return false;

        std::vector<uint8_t> bitmap(ATLAS_W * ATLAS_H);
        stbtt_BakeFontBitmap(fontData.data(), fontOffset, BAKE_SIZE,
                             bitmap.data(), ATLAS_W, ATLAS_H,
                             32, 96, m_charData);

        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ATLAS_W, ATLAS_H, 0,
                     GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        InitShader();
        InitBuffers();
        return true;
    }

    void BeginFrame() {
        m_draws.clear();
    }

    float MeasureWidth(const std::string& text, float fontSize) const {
        float scale = fontSize / BAKE_SIZE;
        float cx = 0, cy = 0;
        for (char c : text) {
            if (c < 32 || c > 127) continue;
            stbtt_aligned_quad q;
            float x = cx, y = cy;
            stbtt_GetBakedQuad(m_charData, ATLAS_W, ATLAS_H, c - 32, &x, &y, &q, 0);
            cx = x;
        }
        return cx * scale;
    }

    void DrawText(float x, float y, const std::string& text, float fontSize, glm::vec4 color) {
        m_draws.push_back({x, y, text, fontSize, color});
    }

    void FlushBatch(const glm::mat4& projection) {
        if (m_draws.empty()) return;

        glUseProgram(m_shader);
        glUniformMatrix4fv(m_projLoc, 1, GL_FALSE, &projection[0][0]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

        for (auto& d : m_draws) {
            m_verts.clear();
            BuildQuads(d.x, d.y, d.text, d.fontSize);
            if (m_verts.empty()) continue;

            glBufferSubData(GL_ARRAY_BUFFER, 0,
                            m_verts.size() * sizeof(TextVertex), m_verts.data());
            glUniform4f(m_colorLoc, d.color.r, d.color.g, d.color.b, d.color.a);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_verts.size()));
        }

        m_draws.clear();
        glBindVertexArray(0);
    }

    void EndFrame(const glm::mat4& projection) {
        FlushBatch(projection);
    }

    ~TextRenderer() {
        if (m_texture) glDeleteTextures(1, &m_texture);
        if (m_vao)     glDeleteVertexArrays(1, &m_vao);
        if (m_vbo)     glDeleteBuffers(1, &m_vbo);
        if (m_shader)  glDeleteProgram(m_shader);
    }

private:
    stbtt_bakedchar m_charData[96]{};
    GLuint m_texture = 0;
    GLuint m_vao = 0, m_vbo = 0;
    GLuint m_shader = 0;
    GLint  m_projLoc = -1, m_colorLoc = -1;

    struct Draw { float x, y; std::string text; float fontSize; glm::vec4 color; };
    std::vector<Draw>       m_draws;
    std::vector<TextVertex> m_verts;

    void BuildQuads(float startX, float startY, const std::string& text, float fontSize) {
        float scale = fontSize / BAKE_SIZE;
        float cx = startX / scale;
        float cy = startY / scale + BAKE_SIZE;

        for (char c : text) {
            if (c < 32 || c > 127) continue;
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(m_charData, ATLAS_W, ATLAS_H, c - 32, &cx, &cy, &q, 0);

            float x0 = q.x0 * scale, y0 = q.y0 * scale;
            float x1 = q.x1 * scale, y1 = q.y1 * scale;

            m_verts.push_back({{x0, y0}, {q.s0, q.t0}});
            m_verts.push_back({{x1, y0}, {q.s1, q.t0}});
            m_verts.push_back({{x0, y1}, {q.s0, q.t1}});

            m_verts.push_back({{x1, y0}, {q.s1, q.t0}});
            m_verts.push_back({{x1, y1}, {q.s1, q.t1}});
            m_verts.push_back({{x0, y1}, {q.s0, q.t1}});
        }
    }

    void InitShader() {
        const char* vert = R"(
            #version 330 core
            layout(location = 0) in vec2 aPos;
            layout(location = 1) in vec2 aUV;
            out vec2 UV;
            uniform mat4 uProjection;
            void main() {
                gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
                UV = aUV;
            }
        )";
        const char* frag = R"(
            #version 330 core
            in vec2 UV;
            out vec4 FragColor;
            uniform sampler2D uTex;
            uniform vec4 uColor;
            void main() {
                float a = texture(uTex, UV).r;
                FragColor = vec4(uColor.rgb, uColor.a * a);
            }
        )";

        auto compile = [](const char* src, GLenum type) -> GLuint {
            GLuint s = glCreateShader(type);
            glShaderSource(s, 1, &src, nullptr);
            glCompileShader(s);
            return s;
        };

        GLuint vs = compile(vert, GL_VERTEX_SHADER);
        GLuint fs = compile(frag, GL_FRAGMENT_SHADER);
        m_shader = glCreateProgram();
        glAttachShader(m_shader, vs);
        glAttachShader(m_shader, fs);
        glLinkProgram(m_shader);
        glDeleteShader(vs);
        glDeleteShader(fs);

        m_projLoc  = glGetUniformLocation(m_shader, "uProjection");
        m_colorLoc = glGetUniformLocation(m_shader, "uColor");
        glUseProgram(m_shader);
        glUniform1i(glGetUniformLocation(m_shader, "uTex"), 0);
    }

    void InitBuffers() {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, 60000 * sizeof(TextVertex), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex),
                              reinterpret_cast<void*>(offsetof(TextVertex, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex),
                              reinterpret_cast<void*>(offsetof(TextVertex, texCoord)));
        glBindVertexArray(0);
    }
};
