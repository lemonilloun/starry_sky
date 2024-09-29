import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

class star_sky:
    def __init__(self, observer_ra, observer_dec, observer_longitude, observer_latitude, fov=[58, 47], magnitude_limit=3.8):
        catalogue = pd.read_csv('Catalogue.csv')  # CSV-файл каталога звёзд
        
        # Извлечение данных: RA, Dec и видимая величина звёзд
        self.observer_ra = observer_ra
        self.observer_dec = observer_dec
        self.observer_longitude = observer_longitude
        self.observer_latitude = observer_latitude
        self.fov = fov
        self.magnitude_limit = magnitude_limit


        self.ra = catalogue['RA']  # Прямое восхождение
        self.dec = catalogue['Dec']  # Склонение х
        self.magnitude = catalogue['Mag']  # Звёздная величина
        self.magnitude_limit = 3.8  # Предельная звёздная величина
    
    def calculate_lst(self, time_hours=0):
        epoch_1991_25_gmst = 198.581921  # GMST на начало эпохи 1991.25 в градусах
        
        lst = (epoch_1991_25_gmst + 360.985647 * time_hours / 24 + 
               np.degrees(self.observer_longitude)) % 360
        
        return lst

    def transform(self, ra, dec, time_hours=0):
        lst = self.calculate_lst(time_hours)
        H = lst - ra

        sin_alt = (np.sin(self.observer_latitude) * np.sin(dec) + 
                   np.cos(self.observer_latitude) * np.cos(dec) * np.cos(H))
        alt = np.arcsin(np.clip(sin_alt, -1, 1))

        cos_A = ((np.sin(dec) - np.sin(self.observer_latitude) * sin_alt) / 
                 (np.cos(self.observer_latitude) * np.cos(alt)))
        sin_A = -np.sin(H) * np.cos(dec) / np.cos(alt)
        
        az = np.arctan2(sin_A, cos_A) % (2 * np.pi)

        return az, alt

    def process_stars(self, time_hours=0):
        # Фильтрация звёзд по звёздной величине
        visible_stars = self.magnitude <= self.magnitude_limit
        ra_visible = self.ra[visible_stars]
        dec_visible = self.dec[visible_stars]
        magnitude_visible = self.magnitude[visible_stars]

        # Преобразование координат
        az, alt = self.transform(ra_visible, dec_visible, time_hours)

        # Фильтрация звёзд над горизонтом
        above_horizon = alt > 0
        az = az[above_horizon]
        alt = alt[above_horizon]
        magnitude_visible = magnitude_visible[above_horizon]

        print(f"Звезд после фильтрации по горизонту: {len(az)}")
        return az, alt, magnitude_visible.values
    
    def filtering_stars(self, A, z, magnitude_visible):
        # Фильтрация звёзд, которые находятся выше горизонта
        above_horizon = z >= 0
        z_above = z[above_horizon]
        A_above = A[above_horizon]
        magnitude_above = magnitude_visible[above_horizon]

        print(f"Звезд выше горизонта: {len(z_above)}")

        # Углы поля зрения (в радианах)
        angle_x = np.radians(self.fov[0] / 2)
        angle_y = np.radians(self.fov[1] / 2)

        # Нахождение границ ksi (lx) и eta (ly)
        lx = np.tan(angle_x)
        ly = np.tan(angle_y)

        # Применение стереографической проекции для координат (A, z)
        x_proj = np.tan((np.pi/2 - z_above) / 2) * np.cos(A_above)  # ksi
        y_proj = np.tan((np.pi/2 - z_above) / 2) * np.sin(A_above)  # eta

        # Звезды, которые находятся в поле зрения
        in_fov = (x_proj >= -lx) & (x_proj <= lx) & (y_proj >= -ly) & (y_proj <= ly)
        x_fov = x_proj[in_fov]
        y_fov = y_proj[in_fov]
        magnitude_fov = magnitude_above[in_fov]

        print(f"Звезд в поле зрения: {len(x_fov)}")
        return (x_fov, y_fov, magnitude_fov), (lx, ly)

    def get_image(self, x_fov, y_fov, bords, mag = None):
        lx, ly = bords
        # Настройка графика
        plt.figure(figsize=(8, 8), facecolor='black')
        plt.scatter(x_fov, y_fov, s=1, color='white')

        # Настройки графика
        plt.xlim([-lx, lx])
        plt.ylim([-ly, ly])
        plt.gca().set_facecolor('black')
        plt.gca().set_aspect('equal')
        plt.axis('off')

        # Показать график
        plt.show()