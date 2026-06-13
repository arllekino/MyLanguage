#pragma once
#include "ViewNode.h"

class VBoxNode : public ViewNode
{
public:
    VBoxNode() = default;

    explicit VBoxNode(float gap)
        : m_gap(gap)
    {
    }

    [[nodiscard]] float GetSpacing() const { return m_gap; }
    void SetSpacing(float spacing) { m_gap = spacing; }

    void Layout(float availableWidth, float availableHeight) override
    {
        float currentY = GetY();
        float maxWidth = 0.0f;

        for (const auto& child : GetChildren())
        {
            if (!child) continue;

            child->SetX(GetX());
            child->SetY(currentY);

            child->Layout(availableWidth, availableHeight);

            currentY += child->GetHeight() + m_gap;

            if (child->GetWidth() > maxWidth)
            {
                maxWidth = child->GetWidth();
            }
        }

        SetWidth(maxWidth);

        float finalHeight = currentY > GetY()
            ? (currentY - GetY() - m_gap)
            : 0.0f;
        SetHeight(finalHeight);
    }

    void Render() override
    {
        for (const auto& child : GetChildren())
        {
            if (child)
            {
                child->Render();
            }
        }
    }

private:
    float m_gap = 0.0f;
};