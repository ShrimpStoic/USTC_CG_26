# HW1: MiniDraw 教学报告

## 1. 问题定义

实现一个交互式画图小程序 MiniDraw，支持绘制多种图形：
- **Line**（直线）— 已提供
- **Rectangle**（矩形）— 已提供
- **Ellipse**（椭圆）— 新增
- **Polygon**（多边形）— 新增
- **Freehand**（自由绘制）— 新增

---

## 2. 核心知识点

### 2.1 类的继承

```
Shape (基类)
├── Line      (直线)
├── Rect      (矩形)
├── Ellipse   (椭圆)   ← 新增
├── Polygon   (多边形) ← 新增
└── Freehand  (自由绘制) ← 新增
```

**继承的好处**：
- 所有形状共享 `draw()` 和 `update()` 接口
- `Canvas` 可以用 `shared_ptr<Shape>` 统一管理所有形状
- 新增形状只需继承 `Shape`，无需修改 `Canvas`

### 2.2 多态（Polymorphism）

```cpp
// Canvas 中统一调用
for (const auto& shape : shape_list_) {
    shape->draw(s);  // 调用的是各子类的 draw()
}
```

**运行时多态**：同一个 `draw()` 调用，根据实际对象类型执行不同逻辑。

### 2.3 ImGui 绘图

```cpp
// 画线
draw_list->AddLine(p1, p2, color, thickness);

// 画椭圆
draw_list->AddEllipse(center, radius, color, rotation, segments, thickness);
```

### 2.4 鼠标交互

| 事件 | 触发条件 | 用途 |
|------|----------|------|
| `IsMouseClicked(Left)` | 左键单击 | 开始/结束绘制 |
| `IsMouseClicked(Right)` | 右键单击 | 完成多边形 |
| `IsMouseDown(Left)` | 左键按住 | 拖拽绘制 |
| `IsItemHovered()` | 鼠标悬停 | 判断是否在画布内 |

---

## 3. 实现要点

### 3.1 Ellipse 类

```cpp
void Ellipse::draw(const Config& config) const {
    float center_x = (start_point_x_ + end_point_x_) * 0.5f;
    float center_y = (start_point_y_ + end_point_y_) * 0.5f;
    float radius_x = std::abs(end_point_x_ - start_point_x_) * 0.5f;
    float radius_y = std::abs(end_point_y_ - start_point_y_) * 0.5f;
    
    draw_list->AddEllipse(center, radius, color, 0, 0, thickness);
}
```

**关键**：用两个对角点定义椭圆的外接矩形。

### 3.2 Polygon 类

```cpp
// 用 vector 存储顶点
std::vector<float> x_list_, y_list_;

// 添加顶点
void Polygon::add_control_point(float x, float y) {
    x_list_.push_back(x);
    y_list_.push_back(y);
}

// 绘制：分解为多段直线
for (size_t i = 0; i < x_list_.size() - 1; i++) {
    draw_list->AddLine(points[i], points[i+1], color, thickness);
}
```

**交互逻辑**：
1. 左键单击 → 创建多边形，添加第一个点
2. 继续左键单击 → 添加更多顶点
3. 右键单击 → 完成多边形

### 3.3 Freehand 类

```cpp
// 每次鼠标移动添加一个点
void Freehand::update(float x, float y) {
    x_list_.push_back(x);
    y_list_.push_back(y);
}
```

**与 Polygon 的区别**：
- Polygon：点击添加顶点
- Freehand：按住拖拽，连续添加点

---

## 4. 踩坑记录

1. **`std::abs` 需要 `<cmath>`**：ellipse.cpp 中忘了 include
2. **右键事件需要单独处理**：ImGui 的 InvisibleButton 默认只捕获左键
3. **多边形的 update 逻辑**：需要更新最后一个点（当前正在拖拽的点）

---

## 5. 延伸思考

- **对象选取**：可以添加 hit test（点击检测）
- **撤销/重做**：用 command pattern 实现
- **图层管理**：z-order、可见性控制
- **导出功能**：保存为 SVG/PNG
