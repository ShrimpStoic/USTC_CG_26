#pragma once

#include <vector>

#include "shape.h"

namespace USTC_CG
{
class Freehand : public Shape
{
   public:
    Freehand() = default;

    virtual ~Freehand() = default;

    // Overrides draw function to implement freehand-specific drawing logic
    void draw(const Config& config) const override;

    // Overrides Shape's update function to add new points during interaction
    void update(float x, float y) override;

   private:
    std::vector<float> x_list_, y_list_;
};
}  // namespace USTC_CG
