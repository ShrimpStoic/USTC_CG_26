// HW2: Abstract Warper class
// Warps points using mathematical mappings, independent of images.
#pragma once

#include <utility>
#include <vector>

namespace USTC_CG
{
class Warper
{
   public:
    virtual ~Warper() = default;

    // Set control point pairs: source -> destination
    void set_points(
        const std::vector<std::pair<float, float>>& src,
        const std::vector<std::pair<float, float>>& dst)
    {
        source_ = src;
        destination_ = dst;
    }

    // Warp a single query point: given (x,y), return the mapped (x',y')
    // Uses INVERSE mapping: for each output pixel q, find where it came from
    virtual std::pair<float, float> warp(float x, float y) const = 0;

   protected:
    std::vector<std::pair<float, float>> source_;      // p_i
    std::vector<std::pair<float, float>> destination_;  // q_i
};
}  // namespace USTC_CG
