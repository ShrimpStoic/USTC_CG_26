# USTC CG 26 — AGENTS.md

## 项目约定
- 作业框架: Framework3D/Ruzino (HW7/HW9), FrameworkRenderformer (HW8)
- 着色器路径: Framework3D/Ruzino/submissions_inner/assignments_inner/nodes_render/shaders/
- C++节点: Framework3D/Ruzino/submissions_inner/assignments_inner/nodes_render/
- HW8代码: FrameworkRenderformer/local_renderformer/

## HW7 关键文件
- blinn_phong.fs: Blinn Phong着色 + Shadow Mapping（主着色器）
- rasterize_impl.fs: 光栅化 + 法线贴图
- node_render_deferred_lighting_gl.cpp: 延迟光照节点（传递uniform）
- shadow_mapping.vs/fs: Shadow Map生成

## HW7 实现要点
1. rasterize_impl.fs: 用TBN矩阵将法线贴图从切线空间转换到世界空间
2. blinn_phong.fs: 实现Blinn-Phong反射模型（ambient + diffuse + specular）
3. ks = metallic*0.8, kd = 1-ks, exponent = (1-roughness)*128
4. Shadow Mapping: 比较片段深度与shadow map中的深度值

## HW8 关键文件
- local_renderformer/models/renderformer.py: 主模型
- local_renderformer/layers/attention.py: 注意力层
- local_renderformer/encodings/: 位置编码

## HW9 关键文件
- mass_spring/MassSpring.cpp: 核心算法
- hw9_node_mass_spring.cpp: Ruzino节点接口
- 需要实现: computeGrad() + step() (半隐式/隐式Euler)
