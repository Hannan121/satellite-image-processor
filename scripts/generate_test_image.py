"""
Generate synthetic star field test image for satellite tracking simulation.
Creates a 3124x3030 image with stars (points) and satellite streaks (lines).
"""
import numpy as np
from PIL import Image

# Configuration
WIDTH = 3124
HEIGHT = 3030
OUTPUT_FILENAME = "Raw_image_Fig1.jpg"

def generate_star_field():
    """Generate synthetic star field with noise, stars, and satellite streaks."""
    
    # Start with dark background
    img = np.zeros((HEIGHT, WIDTH), dtype=np.uint8)
    
    # Add background noise (simulates sensor noise)
    noise_level = 25
    img = img + np.random.randint(0, noise_level, (HEIGHT, WIDTH), dtype=np.uint8)
    
    print(f"[INFO] Creating {WIDTH}x{HEIGHT} synthetic image...")
    
    # Add random stars (bright points)
    num_stars = 800
    print(f"[INFO] Adding {num_stars} stars...")
    for _ in range(num_stars):
        x = np.random.randint(5, WIDTH-5)
        y = np.random.randint(5, HEIGHT-5)
        brightness = np.random.randint(120, 255)
        
        # Star size varies (1x1 to 5x5)
        size = np.random.choice([1, 2, 3], p=[0.5, 0.3, 0.2])
        
        if size == 1:
            img[y, x] = brightness
        elif size == 2:
            img[y-1:y+2, x-1:x+2] = np.maximum(
                img[y-1:y+2, x-1:x+2],
                brightness - 30
            )
            img[y, x] = brightness
        else:  # size == 3
            img[y-2:y+3, x-2:x+3] = np.maximum(
                img[y-2:y+3, x-2:x+3],
                brightness - 50
            )
            img[y-1:y+2, x-1:x+2] = np.maximum(
                img[y-1:y+2, x-1:x+2],
                brightness - 20
            )
            img[y, x] = brightness
    
    # Add satellite streaks (lines)
    num_streaks = 7
    print(f"[INFO] Adding {num_streaks} satellite streaks...")
    for i in range(num_streaks):
        # Random start position
        x_start = np.random.randint(100, WIDTH-100)
        y_start = np.random.randint(100, HEIGHT-100)
        
        # Random length and angle
        length = np.random.randint(80, 300)
        angle = np.random.uniform(0, 2*np.pi)
        
        brightness = np.random.randint(150, 240)
        width = np.random.choice([1, 2, 3], p=[0.3, 0.5, 0.2])
        
        # Draw the streak
        for j in range(length):
            x = int(x_start + j * np.cos(angle))
            y = int(y_start + j * np.sin(angle))
            
            if 0 <= x < WIDTH and 0 <= y < HEIGHT:
                # Vary brightness along streak (brighter in middle)
                position_factor = 1.0 - abs(j - length/2) / (length/2) * 0.3
                pixel_brightness = int(brightness * position_factor)
                
                if width == 1:
                    img[y, x] = max(img[y, x], pixel_brightness)
                else:
                    # Wider streaks
                    for dy in range(-width//2, width//2 + 1):
                        for dx in range(-width//2, width//2 + 1):
                            if 0 <= y+dy < HEIGHT and 0 <= x+dx < WIDTH:
                                dist = np.sqrt(dx**2 + dy**2)
                                falloff = max(0, 1 - dist/width)
                                img[y+dy, x+dx] = max(
                                    img[y+dy, x+dx],
                                    int(pixel_brightness * falloff)
                                )
    
    # Add some gaussian spots (distant out-of-focus objects)
    num_spots = 20
    print(f"[INFO] Adding {num_spots} diffuse spots...")
    for _ in range(num_spots):
        x = np.random.randint(10, WIDTH-10)
        y = np.random.randint(10, HEIGHT-10)
        brightness = np.random.randint(80, 150)
        radius = np.random.randint(3, 8)
        
        # Create gaussian-like blob
        for dy in range(-radius, radius+1):
            for dx in range(-radius, radius+1):
                if 0 <= y+dy < HEIGHT and 0 <= x+dx < WIDTH:
                    dist = np.sqrt(dx**2 + dy**2)
                    if dist <= radius:
                        intensity = int(brightness * np.exp(-(dist/radius)**2))
                        img[y+dy, x+dx] = max(img[y+dy, x+dx], intensity)
    
    return img

def save_image(img, filename):
    """Save numpy array as JPG image."""
    pil_img = Image.fromarray(img, mode='L')
    pil_img.save(filename, quality=95)
    print(f"[SUCCESS] Saved synthetic test image: {filename}")
    print(f"[INFO] Image size: {img.shape[1]}x{img.shape[0]} pixels")
    print(f"[INFO] Pixel value range: {img.min()} to {img.max()}")

def save_binary(img, filename):
    """Save numpy array directly as binary file (ready for C program)."""
    with open(filename, 'wb') as f:
        f.write(img.tobytes())
    print(f"[SUCCESS] Saved binary test data: {filename}")
    print(f"[INFO] File size: {img.nbytes} bytes")
    print(f"[INFO] Ready for C simulation (no conversion needed!)")

if __name__ == "__main__":
    try:
        # Generate synthetic image
        img = generate_star_field()
        
        # Save as JPG (for visual reference)
        save_image(img, OUTPUT_FILENAME)
        
        # Save directly as binary (ready for C program)
        save_binary(img, "input_image.bin")
        
        # Display statistics
        print("\n[STATS]")
        print(f"  Mean pixel value: {img.mean():.2f}")
        print(f"  Std deviation: {img.std():.2f}")
        print(f"  Bright pixels (>100): {np.sum(img > 100)} ({100*np.sum(img > 100)/img.size:.3f}%)")
        print("\n[READY] You can now run:")
        print("  ./satellite_sim.exe       (single-core)")
        print("  ./satellite_dualcore_sim  (dual-core)")
        
    except Exception as e:
        print(f"[ERROR] Failed to generate image: {e}")
