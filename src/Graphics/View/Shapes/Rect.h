#pragma once
#include "../ViewNode.h"

class RectNode : public ViewNode
{
private:
    float m_r = 1.0f;
    float m_g = 1.0f;
    float m_b = 1.0f;
    float m_a = 1.0f;

public:
    RectNode() = default;

    RectNode(float width, float height, float r, float g, float b, float a)
        : m_r(r), m_g(g), m_b(b), m_a(a)
    {
        ViewNode::SetWidth(width);
        ViewNode::SetHeight(height);
    }

    void SetColor(float r, float g, float b, float a)
    {
        m_r = r;
        m_g = g;
        m_b = b;
        m_a = a;
    }

    float GetR() const { return m_r; }
    float GetG() const { return m_g; }
    float GetB() const { return m_b; }
    float GetA() const { return m_a; }

    void Layout(float availableWidth, float availableHeight) override
    {
    }

    void Render() override
    {
        UIDrawRect(
            GetX(),     GetY(), 
            GetWidth(), GetHeight(), 
            m_r,        m_g, 
            m_b,        m_a
        );
    }
};