// HW2: RBF (Radial Basis Function) Warper
// Uses Thin Plate Spline: φ(r) = r² ln(r)
// Solves: f(q) = Σ λ_i φ(|q - q_i|) + a₀ + a₁qx + a₂qy
#pragma once

#include <cmath>
#include <vector>
#include "warper.h"

namespace USTC_CG
{
class RBFWarper : public Warper
{
   public:
    RBFWarper() = default;
    virtual ~RBFWarper() = default;

    // Build the interpolation system from control points.
    // Call this after set_points() and before warp().
    void build_system()
    {
        const int N = static_cast<int>(source_.size());
        if (N == 0)
            return;

        // System size: N + 3 (N lambdas + 3 polynomial coefficients)
        int sz = N + 3;
        A_.assign(sz, std::vector<double>(sz, 0.0));
        rhs_x_.assign(sz, 0.0);
        rhs_y_.assign(sz, 0.0);

        // Fill the Φ block: Φ_ij = φ(|q_i - q_j|)
        for (int i = 0; i < N; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                double dx = destination_[i].first - destination_[j].first;
                double dy = destination_[i].second - destination_[j].second;
                double r = std::sqrt(dx * dx + dy * dy);
                A_[i][j] = tps_phi(r);
            }
        }

        // Fill the P block (polynomial terms): P_i = [1, q_ix, q_iy]
        for (int i = 0; i < N; ++i)
        {
            A_[i][N] = 1.0;
            A_[i][N + 1] = destination_[i].first;
            A_[i][N + 2] = destination_[i].second;

            A_[N][i] = 1.0;
            A_[N + 1][i] = destination_[i].first;
            A_[N + 2][i] = destination_[i].second;
        }
        // Bottom-right 3x3 block is zero (already initialized)

        // RHS: target positions (source coordinates)
        for (int i = 0; i < N; ++i)
        {
            rhs_x_[i] = source_[i].first;
            rhs_y_[i] = source_[i].second;
        }
        // Last 3 entries are zero

        // Solve for x and y separately
        lambda_x_ = solve_linear_system(A_, rhs_x_);
        lambda_y_ = solve_linear_system(A_, rhs_y_);
        built_ = true;
    }

    // Inverse warp: for output point q, find source point p = f(q)
    std::pair<float, float> warp(float x, float y) const override
    {
        if (!built_ || source_.empty())
            return {x, y};

        const int N = static_cast<int>(source_.size());
        double fx = lambda_x_[N] + lambda_x_[N + 1] * x + lambda_x_[N + 2] * y;
        double fy = lambda_y_[N] + lambda_y_[N + 1] * x + lambda_y_[N + 2] * y;

        for (int i = 0; i < N; ++i)
        {
            double dx = x - destination_[i].first;
            double dy = y - destination_[i].second;
            double r = std::sqrt(dx * dx + dy * dy);
            double phi = tps_phi(r);
            fx += lambda_x_[i] * phi;
            fy += lambda_y_[i] * phi;
        }

        return {static_cast<float>(fx), static_cast<float>(fy)};
    }

   private:
    // Thin plate spline basis function: φ(r) = r² ln(r)
    // φ(0) = 0 by convention
    static double tps_phi(double r)
    {
        if (r < 1e-10)
            return 0.0;
        return r * r * std::log(r);
    }

    // Simple Gaussian elimination with partial pivoting
    static std::vector<double> solve_linear_system(
        std::vector<std::vector<double>> A,
        std::vector<double> b)
    {
        int n = static_cast<int>(b.size());

        // Forward elimination with partial pivoting
        for (int col = 0; col < n; ++col)
        {
            // Find pivot
            int max_row = col;
            double max_val = std::abs(A[col][col]);
            for (int row = col + 1; row < n; ++row)
            {
                if (std::abs(A[row][col]) > max_val)
                {
                    max_val = std::abs(A[row][col]);
                    max_row = row;
                }
            }
            std::swap(A[col], A[max_row]);
            std::swap(b[col], b[max_row]);

            // Eliminate below
            for (int row = col + 1; row < n; ++row)
            {
                double factor = A[row][col] / A[col][col];
                for (int k = col; k < n; ++k)
                {
                    A[row][k] -= factor * A[col][k];
                }
                b[row] -= factor * b[col];
            }
        }

        // Back substitution
        std::vector<double> x(n, 0.0);
        for (int i = n - 1; i >= 0; --i)
        {
            double sum = b[i];
            for (int j = i + 1; j < n; ++j)
            {
                sum -= A[i][j] * x[j];
            }
            x[i] = sum / A[i][i];
        }
        return x;
    }

    // Interpolation coefficients
    std::vector<double> lambda_x_, lambda_y_;
    std::vector<std::vector<double>> A_;
    std::vector<double> rhs_x_, rhs_y_;
    bool built_ = false;
};
}  // namespace USTC_CG
