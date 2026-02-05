from PIL import Image

# Load the PNG we created
img = Image.open('C:\\Projects\\OmniAIBench\\src-tauri\\icons\\icon.png')

# Create multiple sizes for the ICO file
img.save(
    'C:\\Projects\\OmniAIBench\\src-tauri\\icons\\icon.ico',
    format='ICO',
    sizes=[(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
)

print("ICO file created successfully!")
