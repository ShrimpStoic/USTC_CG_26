#!/usr/bin/env python3
"""Quick import and smoke test for HW8 implementations."""
import sys
sys.path.insert(0, '.')

# Test imports
print("Testing imports...")
from local_renderformer.models.renderformer import RenderFormer
from local_renderformer.models.view_transformer import ViewTransformer
from local_renderformer.layers.attention import TransformerEncoder, TransformerDecoder, MultiHeadAttention
print("All imports successful!")

import torch
from local_renderformer.models.config import RenderFormerConfig

# ---- NERF path ----
print("\n=== NERF PE path ===")
config = RenderFormerConfig(
    pe_type='nerf',
    vertex_pe_num_freqs=4,
    vn_pe_num_freqs=4,
    use_vn_encoder=True,
    vn_encoder_norm_type='layer_norm',
    texture_channels=3,
    texture_encode_patch_size=4,
    latent_dim=64,
    num_register_tokens=2,
    num_layers=2,
    num_heads=4,
    dim_feedforward=128,
    dropout=0.0,
    activation='gelu',
    norm_type='layer_norm',
    norm_first=True,
    bias=True,
    view_indep_qk_norm=False,
    rope_double_max_freq=False,
    ffn_opt='none',
    vdir_pe_type='nerf',
    vdir_num_freqs=4,
    patch_size=8,
    view_transformer_latent_dim=64,
    view_transformer_n_heads=4,
    view_transformer_n_layers=2,
    view_transformer_ffn_hidden_dim=128,
    view_transformer_include_self_attn=False,
    view_transformer_use_swin_attn=False,
    qk_norm=False,
    include_alpha=False,
    use_dpt_decoder=False,
    encoder_skip_from_layer=None,
    encoder_skip_to_layer=None,
)

model = RenderFormer(config)
print("RenderFormer (nerf) instantiation OK!")

B, N_TRI = 2, 10
tri_vpos_list = torch.randn(B, N_TRI, 9)
texture_patch_list = torch.randn(B, N_TRI, 3, 4, 4)
valid_mask = torch.ones(B, N_TRI, dtype=torch.bool)
vns = torch.randn(B, N_TRI, 9)

print("Testing construct_seq (nerf)...")
seq, mask, pos = model.construct_seq(tri_vpos_list, texture_patch_list, valid_mask, vns)
print(f"  seq={seq.shape}, mask={mask.shape}, pos={pos.shape}")

print("Testing TransformerEncoder.forward (nerf, sdpa)...")
encoder = model.transformer
out = encoder(seq, src_key_padding_mask=mask, triangle_pos=pos)
print(f"  output={out.shape}")

print("Testing ViewTransformer.forward (nerf)...")
vt = model.view_transformer
rays_o = torch.randn(B, 3)
rays_d = torch.randn(B, 32, 32, 3)
with torch.no_grad():
    img = vt(rays_o, rays_d, out, pos, mask)
print(f"  img={img.shape}")

# ---- ROPE path ----
print("\n=== ROPE PE path ===")
config_rope = RenderFormerConfig(
    pe_type='rope',
    vertex_pe_num_freqs=4,  # rope_dim
    vn_pe_num_freqs=4,
    use_vn_encoder=True,
    vn_encoder_norm_type='layer_norm',
    texture_channels=3,
    texture_encode_patch_size=4,
    latent_dim=256,  # must be large enough: rope_dim//2*9 <= latent_dim//num_heads
    num_register_tokens=2,
    num_layers=2,
    num_heads=4,
    dim_feedforward=512,
    dropout=0.0,
    activation='gelu',
    norm_type='layer_norm',
    norm_first=True,
    bias=True,
    view_indep_qk_norm=False,
    rope_double_max_freq=False,
    ffn_opt='none',
    vdir_pe_type='nerf',
    vdir_num_freqs=4,
    patch_size=8,
    view_transformer_latent_dim=256,
    view_transformer_n_heads=4,
    view_transformer_n_layers=2,
    view_transformer_ffn_hidden_dim=512,
    view_transformer_include_self_attn=False,
    view_transformer_use_swin_attn=False,
    qk_norm=False,
    include_alpha=False,
    use_dpt_decoder=False,
    rope_type='triangle',
    encoder_skip_from_layer=None,
    encoder_skip_to_layer=None,
)

model_rope = RenderFormer(config_rope)
print("RenderFormer (rope) instantiation OK!")

print("Testing construct_seq (rope)...")
seq_r, mask_r, pos_r = model_rope.construct_seq(tri_vpos_list, texture_patch_list, valid_mask, vns)
print(f"  seq={seq_r.shape}, mask={mask_r.shape}, pos={pos_r.shape}")

print("Testing TransformerEncoder.forward (rope, sdpa)...")
encoder_rope = model_rope.transformer
out_r = encoder_rope(seq_r, src_key_padding_mask=mask_r, triangle_pos=pos_r)
print(f"  output={out_r.shape}")

print("Testing ViewTransformer.forward (rope)...")
vt_rope = model_rope.view_transformer
with torch.no_grad():
    img_r = vt_rope(rays_o, rays_d, out_r, pos_r, mask_r)
print(f"  img={img_r.shape}")

print("\n=== All tests passed! ===")
