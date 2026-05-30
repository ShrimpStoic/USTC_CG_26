# HW4: DDPM (Denoising Diffusion Probabilistic Models) — 教学报告

## 1. 问题定义

**DDPM**：一种生成模型，通过学习"去噪"过程来生成新图像。

核心思想：
- **前向过程**：逐步给图像加噪声，直到变成纯高斯噪声
- **反向过程**：学习逐步去噪，从纯噪声恢复出清晰图像

类比：把一滴墨水滴入水中（前向/扩散），然后学习如何把墨水"吸回来"（反向/去噪）。

---

## 2. 数学原理

### 2.1 前向扩散过程

给定原始图像 $x_0$，在时间步 $t$ 加噪：

$$q(x_t | x_0) = \mathcal{N}(\sqrt{\bar{\alpha}_t} \cdot x_0, \; (1 - \bar{\alpha}_t) \cdot I)$$

其中：
- $\bar{\alpha}_t = \prod_{s=1}^{t} (1 - \beta_s)$
- $\beta_s$ 是噪声调度（通常从 0.0001 线性增加到 0.02）

**采样公式**（一步到位）：

$$x_t = \sqrt{\bar{\alpha}_t} \cdot x_0 + \sqrt{1 - \bar{\alpha}_t} \cdot \epsilon, \quad \epsilon \sim \mathcal{N}(0, I)$$

### 2.2 反向去噪过程

学习一个神经网络 $\epsilon_\theta(x_t, t)$ 预测噪声，然后：

$$x_{t-1} = \frac{1}{\sqrt{\alpha_t}} \left( x_t - \frac{\beta_t}{\sqrt{1 - \bar{\alpha}_t}} \cdot \epsilon_\theta(x_t, t) \right) + \sigma_t z$$

其中 $z \sim \mathcal{N}(0, I)$，$\sigma_t = \sqrt{\beta_t}$

### 2.3 训练损失

$$L = \mathbb{E}_{t, x_0, \epsilon} \left[ \| \epsilon - \epsilon_\theta(x_t, t) \|^2 \right]$$

即：**预测噪声 vs 实际噪声的 MSE**

---

## 3. 代码架构

### 3.1 整体流程

```
forward_noising.py    → 前向加噪（闭式解）
training_model.py     → 训练循环（Loss + 反向传播）
unet.py              → U-Net 噪声预测网络
sampling.py          → 反向去噪（采样生成）
```

### 3.2 前向加噪（forward_noising.py）

```python
def forward_diffusion_sample(x_0, time_step, device):
    noise = torch.randn_like(x_0)
    
    # 获取 sqrt(alpha_bar_t) 和 sqrt(1 - alpha_bar_t)
    sqrt_alpha_bar_t = get_index_from_list(sqrt_alphas_cumprod, time_step, x_0.shape)
    sqrt_one_minus_alpha_bar_t = get_index_from_list(sqrt_one_minus_alphas_cumprod, time_step, x_0.shape)
    
    # 核心公式: x_t = sqrt(alpha_bar_t) * x_0 + sqrt(1 - alpha_bar_t) * noise
    x_t = sqrt_alpha_bar_t * x_0 + sqrt_one_minus_alpha_bar_t * noise
    
    return x_t, noise
```

**关键**：`get_index_from_list` 将时间步索引扩展到与图像相同的 batch 维度。

### 3.3 训练损失（training_model.py）

```python
def get_loss(model, x_0, t, device):
    # 1. 前向加噪
    x_noisy, noise = forward_diffusion_sample(x_0, t, device)
    
    # 2. 模型预测噪声
    noise_pred = model(x_noisy, t)
    
    # 3. MSE Loss
    loss = F.mse_loss(noise, noise_pred)
    
    return loss
```

训练循环：
```python
for epoch in range(epochs):
    for batch, _ in dataloader:
        optimizer.zero_grad()
        
        # 随机采样时间步
        t = torch.randint(0, T, (batch.shape[0],), device=device)
        
        # 计算损失
        loss = get_loss(model, batch, t, device)
        
        # 反向传播
        loss.backward()
        optimizer.step()
```

### 3.4 单步去噪（sampling.py）

```python
def sample_timestep(model, x, t):
    beta_t = get_index_from_list(betas, t, x.shape)
    sqrt_one_minus_alpha_bar_t = get_index_from_list(sqrt_one_minus_alphas_cumprod, t, x.shape)
    sqrt_recip_alpha_t = get_index_from_list(sqrt_recip_alphas, t, x.shape)
    
    # 预测噪声并去噪
    model_mean = sqrt_recip_alpha_t * (
        x - beta_t * model(x, t) / sqrt_one_minus_alpha_bar_t
    )
    
    if t == 0:
        return model_mean  # 最后一步无噪声
    else:
        noise = torch.randn_like(x)
        return model_mean + torch.sqrt(posterior_var_t) * noise
```

### 3.5 生成图像

```python
def sample_plot_image(model, device, img_size, T):
    # 从纯噪声开始
    img = torch.randn((1, 3, img_size, img_size), device=device)
    
    # 逐步去噪
    for i in range(0, T)[::-1]:
        t = torch.full((1,), i, device=device, dtype=torch.long)
        img = sample_timestep(model, img, t)
        img = torch.clamp(img, -1.0, 1.0)
```

### 3.6 图像修复（RePaint 风格）

```python
def inpaint(model, device, img, mask, t_max=50):
    x = torch.randn_like(img)  # 从纯噪声开始
    
    for i in range(t_max)[::-1]:
        # 1. 去噪一步
        x = sample_timestep(model, x, t)
        
        # 2. 已知区域：用原图 + 对应噪声替换
        if i > 0:
            known_region = sqrt_alpha_bar_t * img + sqrt_one_minus_alpha_bar_t * noise
            x = mask * known_region + (1 - mask) * x
    
    return x
```

---

## 4. U-Net 架构

```
输入: (B, 3, 256, 256)
  ↓
Conv0: 3 → 64
  ↓
Down Block 1: 64 → 128  (Conv + Time Embedding + Downsample)
Down Block 2: 128 → 256
Down Block 3: 256 → 512
Down Block 4: 512 → 1024
  ↓
Up Block 1: 1024+512 → 512  (Concat Skip + Conv + Time Embedding + Upsample)
Up Block 2: 512+256 → 256
Up Block 3: 256+128 → 128
Up Block 4: 128+64 → 64
  ↓
Output Conv: 64 → 3
输出: (B, 3, 256, 256)  # 预测的噪声
```

**时间嵌入**：Sinusoidal Position Embeddings（同 Transformer）

---

## 5. 关键实现细节

### 5.1 时间步采样

训练时随机采样 $t \sim \text{Uniform}(0, T)$，让模型学会在所有噪声 level 上去噪。

### 5.2 数值稳定性

- `get_index_from_list` 自动扩展维度，避免 broadcast 错误
- 生成时 `torch.clamp(-1, 1)` 防止数值溢出

### 5.3 图像修复的核心思想

已知区域的像素 = 原图 + 对应 level 的噪声
未知区域的像素 = 模型去噪的结果

在每一步同时执行，逐步收缩噪声范围。

---

## 6. 运行方式

```bash
cd FrameworkDDPM

# 训练（需要 GPU/MPS，约数小时）
conda activate ustc-cg
python training_model.py

# 生成图像
python sampling.py
```

### 训练参数

| 参数 | 值 |
|------|-----|
| T（时间步） | 300 |
| Batch Size | 1 |
| Epochs | 5000 |
| Learning Rate | 1e-4 |
| Optimizer | Adam |
| 设备 | MPS (Apple Silicon GPU) |

---

## 7. 踩坑记录

### 7.1 BatchNorm vs LayerNorm

U-Net 中注释掉了 BatchNorm（可能因 batch_size=1 不稳定）。实际训练可考虑用 GroupNorm 替代。

### 7.2 模型大小

SimpleUnet 有 **62M 参数**，在 MPS 上训练需要较多显存。Batch size 建议设为 1。

### 7.3 图像尺寸

输入固定为 256×256。U-Net 的 downsample 4 次，所以最小特征图 = 256/16 = 16×16。

---

## 8. 延伸思考

1. **Classifier-Free Guidance**：条件生成（text-to-image）的关键技术
2. **DDIM**：加速采样（1000步→50步），无需训练
3. **Latent Diffusion (Stable Diffusion)**：在潜空间做扩散，大幅降低计算成本
4. **应用**：DALL-E 2、Stable Diffusion、Midjourney 的核心原理
