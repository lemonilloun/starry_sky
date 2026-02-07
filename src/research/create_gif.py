from PIL import Image
import glob

# Загрузка всех изображений
path = "/Users/lehacho/Desktop/bauman/6 sem/НИР/нир-имитатор/conf/"
frames = []
for i in range(1, 17):
    img = path + f"s{i}.png"
    frames.append(Image.open(img))
#frames = [Image.open(img) for img in sorted(glob.glob("/Users/lehacho/Desktop/bauman/6 sem/НИР/нир-имитатор/gif/f*.png"))]

# Сохранение в GIF
frames[0].save(
    'sensor2.gif',
    format='GIF',
    save_all=True,
    append_images=frames[1:],
    duration=300,  # миллисекунд на кадр (100мс = 10 fps)
    loop=0
)