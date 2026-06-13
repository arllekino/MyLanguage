#pragma once
#include <vector>
#include <memory>

class ViewNode
{
public:
    virtual ~ViewNode() = default;

    float GetX() const { return m_x; }
    virtual void SetX(float x) { m_x = x; }

    float GetY() const { return m_y; }
    virtual void SetY(float y) { m_y = y; }

    float GetWidth() const { return m_width; }
    virtual void SetWidth(float width) { m_width = width; }

    float GetHeight() const { return m_height; }
    virtual void SetHeight(float height) { m_height = height; }

    const std::vector<std::shared_ptr<ViewNode>>& GetChildren() const
    {
        return m_children;
    }

    void SetChildren(const std::vector<std::shared_ptr<ViewNode>>& children)
    {
        m_children = children;
    }

    void AddChild(std::shared_ptr<ViewNode> child)
    {
        m_children.push_back(std::move(child));
    }

    virtual void Layout(float availableWidth, float availableHeight) = 0;

    virtual void Render() = 0;

private:
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_width = 0.0f;
    float m_height = 0.0f;

    std::vector<std::shared_ptr<ViewNode>> m_children;
};