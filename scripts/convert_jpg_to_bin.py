from PIL import Image
import numpy as np

# Config matches your system_def.h
TARGET_WIDTH = 3124
TARGET_HEIGHT = 3030
INPUT_FILENAME = "Raw_image_Fig1.jpg"
OUTPUT_FILENAME = "input_image.bin"

def convert_jpg_to_bin():
    try:
        #Open Image
        img = Image.open(INPUT_FILENAME)
        
        # Convert to Grayscale
        img = img.convert('L')
        
        # Resize if necessary
        if img.size != (TARGET_WIDTH, TARGET_HEIGHT):
            print(f"[WARN] Resizing image from {img.size} to ({TARGET_WIDTH}, {TARGET_HEIGHT})")
            img = img.resize((TARGET_WIDTH, TARGET_HEIGHT))
            
        # 4. Save as Raw Binary
        with open(OUTPUT_FILENAME, "wb") as f:
            f.write(img.tobytes())
            
        print(f"[SUCCESS] Saved {OUTPUT_FILENAME}. Ready for C simulation.")
        
    except Exception as e:
        print(f"[ERROR] Could not convert image: {e}")

if __name__ == "__main__":
    convert_jpg_to_bin()