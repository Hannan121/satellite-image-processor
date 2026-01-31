from PIL import Image
import os

INPUT_FILENAME = "output_frame0.pgm"
OUTPUT_FILENAME = "output_visualized.png"

def view_image():
    if not os.path.exists(INPUT_FILENAME):
        print(f"Error: {INPUT_FILENAME} not found. Run your C simulation first.")
        return

    try:
        # 1. Open the PGM file (Pillow handles the headers automatically)
        with Image.open(INPUT_FILENAME) as img:
            print(f"Loaded {INPUT_FILENAME} successfully.")
            print(f"Dimensions: {img.size} | Mode: {img.mode}")

            # 2. Save as PNG (so you can include it in your report)
            img.save(OUTPUT_FILENAME)
            print(f"Saved visualization to: {OUTPUT_FILENAME}")

            # 3. Show the image
            # This opens your default system image viewer
            img.show()

    except Exception as e:
        print(f"Error processing image: {e}")

if __name__ == "__main__":
    view_image()