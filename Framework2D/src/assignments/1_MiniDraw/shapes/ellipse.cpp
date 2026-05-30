#include "ellipse.h"

#include <cmath>
#include <imgui.h>

namespace USTC_CG
{
// Draw the ellipse using ImGui
void Ellipse::draw(const Config& config) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Calculate center and radius from start and end points
    float center_x = (start_point_x_ + end_point_x_) * 0.5f;
    float center_y = (start_point_y_ + end_point_y_) * 0.5f;
    float radius_x = std::abs(end_point_x_ - start_point_x_) * 0.5f;
    float radius_y = std::abs(end_point_y_ - start_point_y_) * 0.5f;

    draw_list->AddEllipse(
        ImVec2(config.bias[0] + center_x, config.bias[1] + center_y),
        ImVec2(radius_x, radius_y),
        IM_COL32(
            config.line_color[0],
            config.line_color[1],
            config.line_color[2],
            config.line_color[3]),
        0.0f,  // rotation
        0,     // num_segments (0 = default)
        config.line_thickness);
}

void Ellipse::update(float x, float y)
{
    end_point_x_ = x;
    end_point_y_ = y;
}
}  // namespace USTC_CG
