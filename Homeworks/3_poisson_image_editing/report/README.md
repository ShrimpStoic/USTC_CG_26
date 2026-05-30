# HW3: Poisson Image Editing — 教学报告

## 1. 问题定义

**Poisson 图像编辑**：将源图像的一个区域"无缝"粘贴到目标图像上，使得边界处的颜色自然过渡，看不出拼接痕迹。

**核心问题**：直接粘贴（Paste）会在区域边界产生明显色差。Poisson 方法通过求解泊松方程，让粘贴区域的梯度（颜色变化趋势）保持源图像的特征，同时边界处平滑过渡到目标图像。

---

## 2. 数学原理

### 2.1 泊松方程

连续形式：

$$\Delta f = \text{div}(\mathbf{v}) \quad \text{in } \Omega$$
$$f = f^* \quad \text{on } \partial\Omega$$

其中：
- $\Omega$ = 粘贴区域（mask 内部）
- $\partial\Omega$ = 区域边界
- $\mathbf{v}$ = 引导场（guidance field）
- $f^*$ = 目标图像（边界条件）

### 2.2 Seamless Cloning 的引导场

对于无缝克隆，引导场就是源图像的梯度：

$$\mathbf{v}_{pq} = s_q - s_p$$

即：保持源图像的颜色变化趋势。

### 2.3 离散化

将连续泊松方程离散为线性方程组。对每个内部像素 $p$：

$$\sum_{q \in N(p)} (f_p - f_q) = \sum_{q \in N(p)} v_{pq}$$

展开：

$$|N(p)| \cdot f_p - \sum_{q \in N(p) \cap \Omega} f_q = \sum_{q \in N(p)} (s_q - s_p) + \sum_{q \in N(p) \cap \partial\Omega} t_q$$

其中：
- $|N(p)|$ = 邻居数量（内部像素为 4）
- 左边 = 未知量（内部像素的 f 值）
- 右边 = 已知量（源图像梯度 + 目标图像边界值）

### 2.4 矩阵形式

$$A \mathbf{f} = \mathbf{b}$$

- $A$ 是稀疏矩阵：每行对角线 = |N(p)|，内部邻居位置 = -1
- $\mathbf{b}$ 是右端向量：包含梯度贡献和边界贡献

**关键特性**：$A$ 是对称正定的（Laplacian 矩阵），保证唯一解。

---

## 3. 代码架构

### 3.1 整体流程

```
1. 用户在 Source 上框选区域 → 生成 binary mask
2. 用户选择 Seamless 模式
3. 用户在 Target 上点击 → 确定粘贴位置
4. 求解泊松方程 → 更新 Target 图像
```

### 3.2 核心算法（Gauss-Seidel 迭代）

```cpp
// 对每个颜色通道 R, G, B 独立求解
for (ch = 0..2) {
    // 初始化：用目标图像值
    f[i] = target_value(mx, my);
    
    // Gauss-Seidel 迭代 200 次
    for (iter = 0..200) {
        for (每个内部像素 i) {
            sum_f = 0;  // 邻居 f 值之和
            rhs = 0;    // 梯度贡献 + 边界贡献
            
            for (4个邻居) {
                if (邻居是内部)
                    sum_f += f[邻居];      // 未知
                else
                    sum_f += target[邻居]; // 已知边界
                
                rhs += source[邻居] - source[p]; // 梯度
            }
            
            f[i] = (sum_f + rhs) / 4;
        }
    }
    
    // 写回图像
    for (每个内部像素 i)
        set_pixel(tx, ty, f[i]);
}
```

### 3.3 内部/边界像素分类

```
Mask 区域:
┌─────────────┐
│ B B B B B B │  B = Boundary (边界，f 已知)
│ B I I I B B │  I = Interior (内部，f 未知)
│ B I I I B B │
│ B B B B B B │
└─────────────┘
```

**判定规则**：像素 p 在 mask 内，且所有 4 个邻居也在 mask 内 → Interior
否则 → Boundary（值取自目标图像）

---

## 4. 关键实现细节

### 4.1 坐标映射

Source mask 坐标 (mx, my) → Target 像素坐标 (tx, ty)：

```cpp
tx = off_x + mx;  // off_x = mouse_x - source_start_x
ty = off_y + my;
```

### 4.2 Gauss-Seidel vs Jacobi

| 方法 | 收敛速度 | 实现复杂度 |
|------|----------|-----------|
| Jacobi | 慢（需要 2 份存储） | 简单 |
| **Gauss-Seidel** | 快（就地更新） | 简单 |
| SOR | 更快 | 需要调参数 |

Gauss-Seidel 的优势：每次更新立即使用最新值，收敛更快。

### 4.3 边界条件处理

边界像素的值直接取自目标图像：

```cpp
if (邻居在 mask 外)
    sum_f += target_pixel(tx + dx, ty + dy);
```

这保证了粘贴区域边界处与目标图像完全一致。

---

## 5. 编译与运行

```bash
cd Framework2D
cmake --build build --target 3_PoissonImageEditing
./bin/3_PoissonImageEditing
```

### 操作流程

1. **File → Open Target** → 选择目标图像
2. **File → Open Source** → 选择源图像
3. 勾选 **Select** → 在 Source 上拖拽框选区域
4. 点击 **Seamless** 按钮
5. 在 Target 上点击 → 无缝粘贴
6. 勾选 **Realtime** → 拖拽鼠标实时更新

---

## 6. Poisson 方程的物理直觉

想象一块无限薄的金属板：
- **Dirichlet 边界条件**：边缘被固定在目标图像的颜色值上
- **引导场**：内部被"力"扭曲，使其梯度匹配源图像
- **解**：金属板的平衡形状 = 无缝粘贴结果

这就是为什么叫"薄板样条"（Thin Plate Spline）——同样的物理模型！

---

## 7. 踩坑记录

### 7.1 内部像素判定

最初只检查 mask 值 > 0，没有检查邻居是否也在 mask 内。导致边界像素也被当作未知量求解，结果发散。

**正确做法**：4 个邻居都在 mask 内 → Interior，否则 → Boundary。

### 7.2 迭代次数

200 次迭代对于中小区域（<100x100）足够收敛。大区域可能需要更多迭代或改用共轭梯度法。

### 7.3 实时编辑性能

Gauss-Seidel 每次迭代 O(N)，200 次迭代 O(200N)。对于 50x50 的 mask（N≈2000），约 40 万次操作，在现代 CPU 上 <10ms，满足实时要求。

---

## 8. 延伸思考

1. **Mixed Gradient**：引导场取源和目标梯度中绝对值较大的那个，适合"移植"场景
2. **共轭梯度法（CG）**：比 Gauss-Seidel 收敛更快，适合大区域
3. **多分辨率**：先在低分辨率求解，再上采样细化，加速大图处理
4. **应用**：Photoshop 的"Content-Aware Fill"、医学图像融合、全景图拼接
