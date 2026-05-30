import torch
from forward_noising import (
    get_index_from_list,
    sqrt_one_minus_alphas_cumprod,
    betas,
    posterior_variance,
    sqrt_recip_alphas,
    forward_diffusion_sample,
    T,
    alphas,
    alphas_cumprod,
)
import matplotlib.pyplot as plt
from dataloader import show_tensor_image
from unet import SimpleUnet
import numpy as np
import cv2 as cv


@torch.no_grad()
def sample_timestep(model, x, t):
    """
    Single step of the reverse (denoising) process.
    
    x_{t-1} = (1/sqrt(alpha_t)) * (x_t - beta_t/sqrt(1-alpha_bar_t) * eps_theta) + sigma_t * z
    
    where:
    - eps_theta = model(x_t, t) is the predicted noise
    - z ~ N(0, I) if t > 0, else z = 0
    - sigma_t = sqrt(posterior_variance_t)
    """
    # Get the betas and alphas for this timestep
    beta_t = get_index_from_list(betas, t, x.shape)
    sqrt_one_minus_alpha_bar_t = get_index_from_list(sqrt_one_minus_alphas_cumprod, t, x.shape)
    sqrt_recip_alpha_t = get_index_from_list(sqrt_recip_alphas, t, x.shape)
    
    # Predict the noise
    model_mean = sqrt_recip_alpha_t * (
        x - beta_t * model(x, t) / sqrt_one_minus_alpha_bar_t
    )
    
    if t == 0:
        # No noise at the last step
        return model_mean
    else:
        posterior_var_t = get_index_from_list(posterior_variance, t, x.shape)
        noise = torch.randn_like(x)
        return model_mean + torch.sqrt(posterior_var_t) * noise


@torch.no_grad()
def sample_plot_image(model, device, img_size, T):
    """
    Generate an image by denoising pure Gaussian noise.
    Starts from x_T ~ N(0, I) and iteratively denoises to x_0.
    """
    # Sample noise
    img = torch.randn((1, 3, img_size, img_size), device=device)
    
    plt.figure(figsize=(15, 15))
    plt.axis("off")
    
    num_images = 10
    stepsize = int(T / num_images)
    
    for i in range(0, T)[::-1]:
        t = torch.full((1,), i, device=device, dtype=torch.long)
        img = sample_timestep(model, img, t)
        # Clamp to [-1, 1] range
        img = torch.clamp(img, -1.0, 1.0)
        
        # Plot intermediate results
        if i % stepsize == 0:
            plt.subplot(1, num_images, int(i / stepsize) + 1)
            show_tensor_image(img.detach().cpu())
    
    plt.savefig("ddpm_generated.png")
    plt.close()
    print("Saved generated image to ddpm_generated.png")


def test_image_generation():
    """
    Load a trained model and generate images.
    """
    device = "mps" if torch.backends.mps.is_available() else "cpu"
    print(f"Using device: {device}")
    
    # Load model
    model = SimpleUnet()
    model.to(device)
    
    # Try to load pre-trained weights
    try:
        model.load_state_dict(torch.load("ddpm_mse_epochs_5000.pth", map_location=device))
        print("Loaded pre-trained model.")
    except FileNotFoundError:
        print("No pre-trained model found. Using random weights.")
    
    model.eval()
    
    img_size = 256
    sample_plot_image(model, device, img_size, T)


@torch.no_grad()
def inpaint(model, device, img, mask, t_max=50):
    """
    Image inpainting using RePaint-style denoising diffusion.
    
    At each reverse step:
    1. Denoise the whole image (like normal sampling)
    2. Replace known regions with the original image + noise
    
    Args:
        model: trained DDPM model
        device: computation device
        img: original image tensor [1, C, H, W] in [-1, 1]
        mask: binary mask [1, C, H, W], 1 = keep, 0 = inpaint
        t_max: maximum timestep for denoising
    """
    # Start from pure noise
    x = torch.randn_like(img)
    
    for i in range(t_max)[::-1]:
        t = torch.full((1,), i, device=device, dtype=torch.long)
        
        # Step 1: Predict noise and denoise
        x = sample_timestep(model, x, t)
        x = torch.clamp(x, -1.0, 1.0)
        
        # Step 2: For known regions, add noise back to match the current noise level
        if i > 0:
            noise = torch.randn_like(img)
            sqrt_alpha_bar_t = torch.sqrt(torch.cumprod(1 - betas, axis=0)[i])
            sqrt_one_minus_alpha_bar_t = torch.sqrt(1 - torch.cumprod(1 - betas, axis=0)[i])
            
            # Known region: add noise to original image at level t
            known_region = sqrt_alpha_bar_t * img + sqrt_one_minus_alpha_bar_t * noise
            
            # Replace known pixels
            x = mask * known_region + (1 - mask) * x
    
    return x


def test_image_inpainting():
    """
    Test image inpainting by masking part of an image and reconstructing it.
    """
    device = "mps" if torch.backends.mps.is_available() else "cpu"
    print(f"Using device: {device}")
    
    # Load model
    model = SimpleUnet()
    model.to(device)
    
    try:
        model.load_state_dict(torch.load("ddpm_mse_epochs_5000.pth", map_location=device))
        print("Loaded pre-trained model.")
    except FileNotFoundError:
        print("No pre-trained model found. Using random weights.")
    
    model.eval()
    
    # Create a test image (use a sample from dataset)
    from torchvision import transforms
    import torchvision
    
    img_size = 256
    data_transform = transforms.Compose([
        transforms.Resize((img_size, img_size)),
        transforms.ToTensor(),
        transforms.Lambda(lambda t: (t * 2) - 1),
    ])
    
    try:
        dataset = torchvision.datasets.ImageFolder(root="./datasets-1/test", transform=data_transform)
        img, _ = dataset[0]
        img = img.unsqueeze(0).to(device)
    except:
        # Fallback: use random noise
        img = torch.randn((1, 3, img_size, img_size), device=device)
    
    # Create a center mask (keep edges, inpaint center)
    mask = torch.ones_like(img)
    mask[:, :, img_size//4:3*img_size//4, img_size//4:3*img_size//4] = 0
    
    # Apply mask
    masked_img = img * mask
    
    # Inpaint
    result = inpaint(model, device, masked_img, mask)
    
    # Save results
    fig, axes = plt.subplots(1, 3, figsize=(15, 5))
    
    plt.subplot(1, 3, 1)
    show_tensor_image(img.cpu())
    plt.title("Original")
    
    plt.subplot(1, 3, 2)
    show_tensor_image(masked_img.cpu())
    plt.title("Masked")
    
    plt.subplot(1, 3, 3)
    show_tensor_image(result.cpu())
    plt.title("Inpainted")
    
    plt.savefig("ddpm_inpainting.png")
    plt.close()
    print("Saved inpainting result to ddpm_inpainting.png")


if __name__ == "__main__":
    test_image_generation()
    test_image_inpainting()
