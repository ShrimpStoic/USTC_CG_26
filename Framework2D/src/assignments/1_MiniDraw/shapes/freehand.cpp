#include "freehand.h"

#include <imgui.h>

namespace USTC_CG
{
// Draw the freehand path using ImGui
void Freehand::draw(const Config& config) const
{
    if (x_list_.size() < 2)
        return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Draw each segment of the freehand path as a line
    for (size_t i = 0; i < x_list_.size() - 1; i++)
    {
        draw_list->AddLine(
            ImVec2(
                config.bias[0] + x_list_[i], config.bias[1] + y_list_[i]),
            ImVec2(
                config.bias[0] + x_list_[i + 1],
                config.bias[1] + y_list_[i + 1]),
            IM_COL32(
                config.line_color[0],
                config.line_color[1],
                config.line_color[2],
                config.line_color[3]),
            config.line_thickness);
    }
}

void Freehand::update(float x, float y)
{
    // Add new point to the path
    x_list_.push_back(x);
    y_list_.push_back(y);
}
}  // namespace USTC_CG
