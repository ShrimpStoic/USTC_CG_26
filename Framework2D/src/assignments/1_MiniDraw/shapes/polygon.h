#pragma once

#include <vector>

#include "shape.h"

namespace USTC_CG
{
class Polygon : public Shape
{
   public:
    Polygon() = default;

    Polygon(std::vector<float> x_list, std::vector<float> y_list)
        : x_list_(std::move(x_list)), y_list_(std::move(y_list))
    {
    }

    virtual ~Polygon() = default;

    // Overrides draw function to implement polygon-specific drawing logic
    void draw(const Config& config) const override;

    // Overrides Shape's update function to adjust the last point during
    // interaction
    void update(float x, float y) override;

    // Adds a control point (vertex) to the polygon
    void add_control_point(float x, float y) override;

   private:
    std::vector<float> x_list_, y_list_;
};
}  // namespace USTC_CG
