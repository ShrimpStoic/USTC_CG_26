# HW2: Image Warping — 教学报告

## 1. 问题定义

**Image Warping（图像变形）**：给定一张源图像和若干控制点对（源点 → 目标点），生成一张新图像，使得控制点被"拉"到目标位置，其余像素平滑插值。

核心挑战：如何从少量控制点对推断出**整张图像**的变形场？

### 两种基本策略

| 策略 | 原理 | 问题 |
|------|------|------|
| **前向映射** f(p)→p' | 对源图像每个像素计算目标位置 | 产生空洞（多个源点映射到同一目标） |
| **逆向映射** f⁻¹(q)→p | 对目标图像每个像素反查源位置 | 无空洞，计算更稳定 |

**本作业采用逆向映射**：对输出图像的每个像素 (x,y)，用插值函数找到它在源图像中的对应位置 (sx,sy)，然后采样源图像。

---

## 2. 数学原理

### 2.1 IDW — 反距离加权插值

** Shepard's Method**：对于查询点 q，位移量 d(q) 是已知位移量的加权平均：

$$d(q) = \frac{\sum_{i=1}^{N} w_i \cdot d_i}{\sum_{i=1}^{N} w_i}$$

其中权重：

$$w_i = \frac{1}{|q - q_i|^\mu}$$

- $q_i$ = 目标控制点位置
- $d_i = p_i - q_i$ = 已知位移（源点 - 目标点）
- $\mu$ = 幂次参数（通常取 2）

**逆向映射版本**：对于输出像素 q，直接计算源位置：

$$p(q) = \frac{\sum_{i=1}^{N} w_i \cdot p_i}{\sum_{i=1}^{N} w_i}$$

其中 $w_i = 1/|q - q_i|^\mu$

**关键特性**：
- 距离控制点越近，影响越大
- 离所有控制点都很远时，趋向于无位移
- 计算简单，O(N) 每像素

### 2.2 RBF — 径向基函数插值（Thin Plate Spline）

**核心思想**：用径向基函数 φ(r) 构建平滑插值：

$$f(q) = \sum_{i=1}^{N} \lambda_i \cdot \phi(|q - q_i|) + a_0 + a_1 q_x + a_2 q_y$$

**Thin Plate Spline（薄板样条）**：

$$\phi(r) = r^2 \ln(r), \quad \phi(0) = 0$$

这个名字来自物理直觉：想象一块无限薄的金属板，你在若干点上施加力将其弯曲，薄板的形状就是 TPS 的解。

**求解系数**：将 N 个控制点代入，得到 N+3 个方程：

$$\sum_{j=1}^{N} \lambda_j \cdot \phi(|q_i - q_j|) + a_0 + a_1 q_{ix} + a_2 q_{iy} = p_{ix} \quad (i=1..N)$$

加上平滑约束：

$$\sum_{j=1}^{N} \lambda_j = 0, \quad \sum_{j=1}^{N} \lambda_j q_{jx} = 0, \quad \sum_{j=1}^{N} \lambda_j q_{jy} = 0$$

写成矩阵形式：

$$\begin{bmatrix} \Phi & P \\ P^T & 0 \end{bmatrix} \begin{bmatrix} \lambda \\ a \end{bmatrix} = \begin{bmatrix} p \\ 0 \end{bmatrix}$$

其中 $\Phi_{ij} = \phi(|q_i - q_j|)$，$P$ 是多项式项矩阵。

**关键特性**：
- 保证**全局平滑**（C¹ 连续）
- 控制点处精确插值
- 计算：构建 O(N²) + 求解 O(N³)，每像素 O(N)

---

## 3. 代码架构

### 3.1 类设计（面向对象三件套）

```
Warper (抽象基类)
├── IDWWarper  (反距离加权)
└── RBFWarper  (薄板样条)
```

**封装**：每个 Warper 独立管理自己的数学逻辑
**继承**：统一的 `warp(x,y)` 接口
**多态**：`warping_widget.cpp` 通过基类指针调用，无需关心具体算法

### 3.2 Warper 基类

```cpp
class Warper {
public:
    void set_points(src, dst);           // 设置控制点对
    virtual pair<float,float> warp(x,y) = 0;  // 逆向映射
protected:
    vector<pair<float,float>> source_, destination_;
};
```

### 3.3 IDWWarper 实现

```cpp
pair<float,float> warp(float x, float y) const {
    float w_sum_x = 0, w_sum_y = 0, w_sum = 0;
    for (i = 0..N) {
        float dist = distance(q, destination_[i]);
        if (dist < 1e-6) return source_[i];  // 精确命中
        float w = 1.0 / pow(dist, mu_);
        w_sum_x += w * source_[i].first;
        w_sum_y += w * source_[i].second;
        w_sum += w;
    }
    return {w_sum_x / w_sum, w_sum_y / w_sum};
}
```

### 3.4 RBFWarper 实现

```cpp
// 1. 构建线性系统
void build_system() {
    // Φ_ij = r² ln(r), P = [1, x, y]
    // 求解 [Φ P; Pᵀ 0] [λ; a] = [p; 0]
    lambda_x_ = solve(A, rhs_x);
    lambda_y_ = solve(A, rhs_y);
}

// 2. 逆向映射
pair<float,float> warp(float x, float y) {
    fx = a0 + a1*x + a2*y;
    for (i = 0..N)
        fx += λ_i * φ(|q - q_i|);
    return {fx, fy};
}
```

### 3.5 逆向映射循环

```cpp
// 在 warping_widget.cpp 中
for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
        auto [sx, sy] = warper.warp(x, y);  // 反查源位置
        if (in_bounds(sx, sy))
            warped.set_pixel(x, y, source.get_pixel(sx, sy));
    }
}
```

---

## 4. 关键实现细节

### 4.1 避免除零

IDW 中当查询点恰好在控制点上时，距离为 0，权重无穷大。解决方案：

```cpp
if (dist < 1e-6f)
    return source_[i];  // 直接返回精确值
```

### 4.2 Thin Plate Spline 的 φ(0)

$r^2 \ln(r)$ 在 r=0 时为 0·(-∞)，极限为 0：

```cpp
static double tps_phi(double r) {
    if (r < 1e-10) return 0.0;
    return r * r * std::log(r);
}
```

### 4.3 线性方程组求解

用 Gaussian 消元 + 列主元选取，O(N³)：

```cpp
// 前向消元（带选主元）
for (col = 0..n) {
    找最大 |A[row][col]| → 交换行
    for (row = col+1..n)
        消去 A[row][col]
}
// 回代
for (i = n-1..0)
    x[i] = (b[i] - Σ A[i][j]*x[j]) / A[i][i]
```

N 通常 < 20（控制点数量），所以朴素消元完全够用。

---

## 5. 编译与运行

```bash
cd Framework2D
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target 2_ImageWarping
./bin/2_ImageWarping
```

### 操作流程

1. **File → Open Image File** → 选择一张图片
2. **Select Points** → 在图片上拖拽画线（起点=蓝，终点=绿，连线=红）
3. 选择变形类型：**Fisheye / IDW / RBF**
4. **Warping** → 执行变形
5. **Restore** → 恢复原图

---

## 6. IDW vs RBF 对比

| 特性 | IDW | RBF (TPS) |
|------|-----|-----------|
| 平滑度 | C⁰（控制点处可能有折痕） | C¹（全局平滑） |
| 计算复杂度 | O(N) 每像素 | O(N) 每像素（预计算后） |
| 预计算 | 无 | O(N²) 构建 + O(N³) 求解 |
| 适用场景 | 快速变形，局部控制 | 需要平滑过渡的变形 |
| 物理直觉 | 距离衰减影响 | 薄板弯曲能量最小 |

---

## 7. 踩坑记录

### 7.1 Eigen 依赖

框架没有包含 Eigen，而 RBF 需要解线性方程组。解决方案：手写 Gaussian 消元器，对于小规模问题（N<20）性能完全够用。

### 7.2 逆向 vs 前向映射

最初考虑前向映射（源→目标），但会产生空洞。改用逆向映射（目标→源）后问题消失。

### 7.3 TPS 的 φ(r) 在 r=0

`r² log(r)` 在 r=0 的数学极限是 0，但直接计算会得到 NaN。必须特判。

---

## 8. 延伸思考

1. **其他 RBF 核函数**：Gaussian $e^{-r^2/\sigma^2}$、Multiquadric $\sqrt{r^2+c^2}$、MQ 的局部性更好
2. **Moving Least Squares (MLS)**：另一种流行的变形方法，更适合大规模控制点
3. **Mesh-based Warping**：将图像三角化，在三角形内做仿射变换
4. **应用**：人脸变形、全景图拼接、医学图像配准
