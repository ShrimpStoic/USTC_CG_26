# USTC CG 26 — 剩余作业执行计划

## 项目概览
完成 USTC 计算机图形学课程剩余 3 个作业（HW7/HW8/HW9），使用 ralph-loop-hermes 框架。

## 作业清单

### HW7: Shader Programming（光栅化 & 路径追踪）
- **框架**: Framework3D/Ruzino
- **语言**: GLSL + C++
- **核心任务**:
  - S1: Blinn Phong 着色 + 法线贴图（blinn_phong.fs + rasterize_impl.fs）
  - S2: Shadow Mapping（shadow_mapping.vs/fs + deferred shading）
  - S3: 可选 — PCSS / SSAO

### HW8: RenderFormer（神经渲染）
- **框架**: FrameworkRenderformer (PyTorch)
- **语言**: Python
- **核心任务**:
  - S4: 编码模块实现（Triangle Embedding, Ray Bundle Embedding, Relative Spatial P.E.）
  - S5: 注意力机制实现（Self-Attention + Cross-Attention）
  - S6: 训练验证（PSNR > 15）

### HW9: Mass Spring（质点弹簧仿真）
- **框架**: Framework3D/Ruzino
- **语言**: C++ (Eigen)
- **核心任务**:
  - S7: 半隐式 Euler 积分（computeGrad + step）
  - S8: 隐式 Euler 积分（线性系统求解）
  - S9: 可选 — 碰撞处理 / Liu 2013 加速

## 执行策略

### 批次划分
```
批次1 (HW7, 并行):
  S1: Blinn Phong + Normal Mapping → coder
  S2: Shadow Mapping → coder

批次2 (HW8, 串行):
  S4: 编码模块 → coder
  S5: 注意力机制 → coder (依赖S4)
  S6: 训练验证 → orchestrator执行

批次3 (HW9, 串行):
  S7: 半隐式 Euler → coder
  S8: 隐式 Euler → coder (依赖S7)
```

### 模型配置
- **Orchestrator**: mimo-v2.5 (default profile) — 任务分解、验证、协调
- **Workers**: MiniMax-M2.7 (coder profile) — 代码实现
- **验证**: 编译/运行由 orchestrator 直接执行

### 验证方式
- HW7: 编译通过 + 渲染截图对比
- HW8: 训练收敛 + PSNR > 15
- HW9: 编译通过 + 仿真动画输出

## 预期产出
- HW7: 修改后的 shader 文件 + 渲染结果截图
- HW8: 修改后的 Python 模块 + 训练日志 + PSNR 报告
- HW9: 修改后的 C++ 代码 + 仿真 GIF/MP4
