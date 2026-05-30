// HW2: IDW (Inverse Distance Weighting) Warper
// Reference: Shepard's method
#pragma once

#include "warper.h"

namespace USTC_CG
{
class IDWWarper : public Warper
{
   public:
    IDWWarper() = default;
    virtual ~IDWWarper() = default;

    // Set the power parameter μ (default 2)
    void set_power(float mu) { mu_ = mu; }

    // Inverse warp: for output point q, find source point p
    std::pair<float, float> warp(float x, float y) const override
    {
        if (source_.empty())
            return {x, y};

        float w_sum_x = 0, w_sum_y = 0, w_sum = 0;

        for (size_t i = 0; i < source_.size(); ++i)
        {
            // Distance from query point q=(x,y) to destination point q_i
            float dx = x - destination_[i].first;
            float dy = y - destination_[i].second;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < 1e-6f)
            {
                // Query is exactly on a control point → return source directly
                return source_[i];
            }

            float w = 1.0f / std::pow(dist, mu_);
            w_sum_x += w * source_[i].first;
            w_sum_y += w * source_[i].second;
            w_sum += w;
        }

        return {w_sum_x / w_sum, w_sum_y / w_sum};
    }

   private:
    float mu_ = 2.0f;  // IDW power parameter
};
}  // namespace USTC_CG
