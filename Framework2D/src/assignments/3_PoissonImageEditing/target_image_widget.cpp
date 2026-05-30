#include "target_image_widget.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <vector>

namespace USTC_CG
{
using uchar = unsigned char;

TargetImageWidget::TargetImageWidget(
    const std::string& label,
    const std::string& filename)
    : ImageWidget(label, filename)
{
    if (data_)
        back_up_ = std::make_shared<Image>(*data_);
}

void TargetImageWidget::draw()
{
    // Draw the image
    ImageWidget::draw();
    // Invisible button for interactions
    ImGui::SetCursorScreenPos(position_);
    ImGui::InvisibleButton(
        label_.c_str(),
        ImVec2(
            static_cast<float>(image_width_),
            static_cast<float>(image_height_)),
        ImGuiButtonFlags_MouseButtonLeft);
    bool is_hovered_ = ImGui::IsItemHovered();

    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        mouse_click_event();
    }
    mouse_move_event();
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        mouse_release_event();
    }
}

void TargetImageWidget::set_source(std::shared_ptr<SourceImageWidget> source)
{
    source_image_ = source;
}

void TargetImageWidget::set_realtime(bool flag)
{
    flag_realtime_updating = flag;
}

void TargetImageWidget::restore()
{
    *data_ = *back_up_;
    update();
}

void TargetImageWidget::set_paste()
{
    clone_type_ = kPaste;
}

void TargetImageWidget::set_seamless()
{
    clone_type_ = kSeamless;
}

// =====================================================================
// Poisson Seamless Cloning — Gauss-Seidel iterative solver
//
// For each interior pixel p in the mask Ω, solve:
//   |N(p)|·f_p - Σ f_q = Σ (s_q - s_p) + Σ t_q
//   ──────────────────   ─────────────────   ─────
//     (unknowns)          (guidance field)   (boundary)
//
// We solve per-channel (R, G, B) independently.
// =====================================================================
void TargetImageWidget::clone()
{
    if (data_ == nullptr || source_image_ == nullptr ||
        source_image_->get_region_mask() == nullptr)
        return;

    std::shared_ptr<Image> mask = source_image_->get_region_mask();

    switch (clone_type_)
    {
        case USTC_CG::TargetImageWidget::kDefault: break;
        case USTC_CG::TargetImageWidget::kPaste:
        {
            restore();

            for (int x = 0; x < mask->width(); ++x)
            {
                for (int y = 0; y < mask->height(); ++y)
                {
                    int tar_x =
                        static_cast<int>(mouse_position_.x) + x -
                        static_cast<int>(source_image_->get_position().x);
                    int tar_y =
                        static_cast<int>(mouse_position_.y) + y -
                        static_cast<int>(source_image_->get_position().y);
                    if (0 <= tar_x && tar_x < image_width_ && 0 <= tar_y &&
                        tar_y < image_height_ && mask->get_pixel(x, y)[0] > 0)
                    {
                        data_->set_pixel(
                            tar_x,
                            tar_y,
                            source_image_->get_data()->get_pixel(x, y));
                    }
                }
            }
            break;
        }
        case USTC_CG::TargetImageWidget::kSeamless:
        {
            restore();

            // Offset: where the source region is placed on the target
            int off_x = static_cast<int>(mouse_position_.x) -
                         static_cast<int>(source_image_->get_position().x);
            int off_y = static_cast<int>(mouse_position_.y) -
                         static_cast<int>(source_image_->get_position().y);

            // Source data and mask
            auto src = source_image_->get_data();

            // 1. Collect interior pixels and build index mapping
            //    key: (x, y) in mask space → value: equation index
            std::map<std::pair<int, int>, int> idx_map;
            std::vector<std::pair<int, int>> interior;

            for (int y = 0; y < mask->height(); ++y)
            {
                for (int x = 0; x < mask->width(); ++x)
                {
                    if (mask->get_pixel(x, y)[0] > 0)
                    {
                        // Check if at least one neighbor is outside the mask
                        // (boundary pixel) — we only solve for interior
                        bool is_interior = true;
                        const int dx[4] = {-1, 1, 0, 0};
                        const int dy[4] = {0, 0, -1, 1};
                        for (int d = 0; d < 4; ++d)
                        {
                            int nx = x + dx[d], ny = y + dy[d];
                            if (nx < 0 || nx >= mask->width() || ny < 0 ||
                                ny >= mask->height() ||
                                mask->get_pixel(nx, ny)[0] == 0)
                            {
                                is_interior = false;
                                break;
                            }
                        }
                        if (is_interior)
                        {
                            idx_map[{x, y}] = static_cast<int>(interior.size());
                            interior.push_back({x, y});
                        }
                    }
                }
            }

            int N = static_cast<int>(interior.size());
            if (N == 0)
                break;

            // 2. Solve Poisson equation per channel using Gauss-Seidel
            const int dx[4] = {-1, 1, 0, 0};
            const int dy[4] = {0, 0, -1, 1};
            const int max_iter = 200;

            for (int ch = 0; ch < 3; ++ch)  // R, G, B
            {
                // Initialize with target values (shifted)
                std::vector<double> f(N, 0.0);
                for (int i = 0; i < N; ++i)
                {
                    int mx = interior[i].first;
                    int my = interior[i].second;
                    int tx = off_x + mx;
                    int ty = off_y + my;
                    if (tx >= 0 && tx < image_width_ && ty >= 0 &&
                        ty < image_height_)
                    {
                        f[i] = data_->get_pixel(tx, ty)[ch];
                    }
                }

                // Gauss-Seidel iteration
                for (int iter = 0; iter < max_iter; ++iter)
                {
                    for (int i = 0; i < N; ++i)
                    {
                        int mx = interior[i].first;
                        int my = interior[i].second;
                        int tx = off_x + mx;
                        int ty = off_y + my;

                        double sum_f = 0;
                        int count = 0;
                        double rhs = 0;

                        for (int d = 0; d < 4; ++d)
                        {
                            int nx = mx + dx[d];
                            int ny = my + dy[d];
                            int ntx = tx + dx[d];
                            int nty = ty + dy[d];

                            count++;  // |N(p)|

                            if (idx_map.count({nx, ny}))
                            {
                                // Interior neighbor: unknown
                                sum_f += f[idx_map[{nx, ny}]];
                            }
                            else
                            {
                                // Boundary neighbor: known from target
                                if (ntx >= 0 && ntx < image_width_ &&
                                    nty >= 0 && nty < image_height_)
                                {
                                    sum_f += data_->get_pixel(ntx, nty)[ch];
                                }
                            }

                            // Guidance field: s_q - s_p
                            if (nx >= 0 && nx < src->width() && ny >= 0 &&
                                ny < src->height())
                            {
                                rhs += src->get_pixel(nx, ny)[ch];
                            }
                            else
                            {
                                rhs += src->get_pixel(mx, my)[ch];
                            }
                        }

                        rhs -= count * src->get_pixel(mx, my)[ch];

                        // Add boundary contributions to RHS
                        for (int d = 0; d < 4; ++d)
                        {
                            int nx = mx + dx[d];
                            int ny = my + dy[d];
                            int ntx = tx + dx[d];
                            int nty = ty + dy[d];

                            if (!idx_map.count({nx, ny}))
                            {
                                if (ntx >= 0 && ntx < image_width_ &&
                                    nty >= 0 && nty < image_height_)
                                {
                                    rhs += data_->get_pixel(ntx, nty)[ch];
                                }
                            }
                        }

                        f[i] = (sum_f + rhs) / count;
                    }
                }

                // 3. Write solved values back to the image
                for (int i = 0; i < N; ++i)
                {
                    int mx = interior[i].first;
                    int my = interior[i].second;
                    int tx = off_x + mx;
                    int ty = off_y + my;

                    if (tx >= 0 && tx < image_width_ && ty >= 0 &&
                        ty < image_height_)
                    {
                        int val = static_cast<int>(
                            std::clamp(f[i], 0.0, 255.0));
                        auto pixel = data_->get_pixel(tx, ty);
                        pixel[ch] = static_cast<uchar>(val);
                        data_->set_pixel(tx, ty, pixel);
                    }
                }
            }

            break;
        }
        default: break;
    }

    update();
}

void TargetImageWidget::mouse_click_event()
{
    edit_status_ = true;
    mouse_position_ = mouse_pos_in_canvas();
    clone();
}

void TargetImageWidget::mouse_move_event()
{
    if (edit_status_)
    {
        mouse_position_ = mouse_pos_in_canvas();
        if (flag_realtime_updating)
            clone();
    }
}

void TargetImageWidget::mouse_release_event()
{
    if (edit_status_)
    {
        edit_status_ = false;
    }
}

ImVec2 TargetImageWidget::mouse_pos_in_canvas() const
{
    ImGuiIO& io = ImGui::GetIO();
    return ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y);
}
}  // namespace USTC_CG
