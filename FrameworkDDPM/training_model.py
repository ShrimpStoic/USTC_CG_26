from forward_noising import forward_diffusion_sample
from unet import SimpleUnet
from dataloader import load_transformed_dataset
import torch.nn.functional as F
import torch
from torch.optim import Adam
import logging
from tqdm import trange
import cv2 as cv

logging.basicConfig(level=logging.INFO)


def get_loss(model, x_0, t, device):
    """
    Compute the DDPM training loss.
    
    Loss = E[||epsilon - epsilon_theta(x_t, t)||^2]
    
    where:
    - epsilon is the actual noise added
    - epsilon_theta is the model's predicted noise
    - x_t = sqrt(alpha_bar_t) * x_0 + sqrt(1 - alpha_bar_t) * epsilon
    """
    # Forward diffusion: add noise to x_0 at timestep t
    x_noisy, noise = forward_diffusion_sample(x_0, t, device)
    
    # Model predicts the noise
    noise_pred = model(x_noisy, t)
    
    # MSE loss between predicted and actual noise
    loss = F.mse_loss(noise, noise_pred)
    
    return loss


if __name__ == "__main__":
    model = SimpleUnet()
    T = 300
    BATCH_SIZE = 1
    epochs = 5000

    dataloader = load_transformed_dataset(batch_size=BATCH_SIZE)

    device = "mps" if torch.backends.mps.is_available() else "cpu"
    logging.info(f"Using device: {device}")
    model.to(device)
    optimizer = Adam(model.parameters(), lr=1e-4)

    for epoch in range(epochs):
        for batch_idx, (batch, _) in enumerate(dataloader):
            optimizer.zero_grad()

            # Sample random timesteps for each image in the batch
            t = torch.randint(0, T, (batch.shape[0],), device=device).long()
            
            # Compute loss
            loss = get_loss(model, batch, t, device)
            
            # Backpropagation
            loss.backward()
            optimizer.step()

            if batch_idx % 50 == 0:
                logging.info(f"Epoch {epoch} | Batch index {batch_idx:03d} Loss: {loss.item():.6f}")

    torch.save(model.state_dict(), f"./ddpm_mse_epochs_{epochs}.pth")
