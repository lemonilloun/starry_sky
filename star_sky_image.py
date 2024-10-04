import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

class star_sky:
    def __init__(self, observer_ra, observer_dec, fov=[58, 47], magnitude_limit=3.8):
        catalogue = pd.read_csv('Catalogue.csv')  # CSV-файл каталога звёзд
        
        # Извлечение данных: RA, Dec и видимая величина звёзд
        self.observer_ra = observer_ra
        self.observer_dec = observer_dec
        self.fov = fov
        self.magnitude_limit = magnitude_limit


        self.ra = np.radians(catalogue['RA'])  # Прямое восхождение в радианах
        self.dec = np.radians(catalogue['Dec'])  # Склонение в радианах
        self.magnitude = catalogue['Mag']  # Звёздная величина
        self.magnitude_limit = magnitude_limit  # Предельная звёздная величина

    def transform(self, delta, alpha):
        """
        Преобразование экваториальных координат в горизонтальные.

        :param phi: Широта наблюдателя (радианы)
        :param t0: Местное звёздное время наблюдателя (радианы)
        :param alpha: Прямое восхождение звезды (радианы)
        :param delta: Склонение звезды (радианы)
        :return: Высота (h) и азимут (a) звезды
        """

        phi = self.observer_dec
        t0 = self.observer_ra

        # Часовой угол
        t = alpha - t0

        # Зенитное расстояние
        z = np.arccos(np.sin(delta) * np.sin(phi) + np.cos(delta) * np.cos(phi) * np.cos(t))
        # Высота светила
        h = np.pi / 2 - z

        # Азимут
        sin_a1 = np.cos(delta) * np.sin(t) / np.sin(z)
        cos_a2 = (-np.sin(delta) * np.cos(phi) + np.cos(delta) * np.sin(phi) * np.cos(t)) / np.sin(z)

        # Обработка азимута в зависимости от знака sin_a1
        A = np.where(sin_a1 > 0, np.arccos(cos_a2), 2 * np.pi - np.arccos(cos_a2))
        A = np.where(np.abs(sin_a1) <= 1e-6, 0, A)

        return A, h

    def process_stars(self):
        # Фильтрация звёзд по звёздной величине
        visible_stars = self.magnitude <= self.magnitude_limit
        ra_visible = self.ra[visible_stars]
        dec_visible = self.dec[visible_stars]
        magnitude_visible = self.magnitude[visible_stars]

        # Преобразование координат
        az, alt = self.transform(ra_visible, dec_visible)

        # Фильтрация звёзд над горизонтом
        above_horizon = alt > 0
        az = az[above_horizon]
        alt = alt[above_horizon]
        magnitude_visible = magnitude_visible[above_horizon]

        print(f"Звезд выше горизонта: {len(alt)}")

        return az, alt, magnitude_visible.values
    
    def filtering_stars(self, A_above, z_above, magnitude_visible):
        # Фильтрация звёзд, которые находятся выше горизонта
    
        # Углы поля зрения (в радианах)
        angle_x = np.radians(self.fov[0] / 2)
        angle_y = np.radians(self.fov[1] / 2)

        # Нахождение границ ksi (lx) и eta (ly)
        lx = abs((np.sin(np.pi / 2 + angle_x) * np.cos(np.pi / 2) - np.cos(np.pi / 2 + angle_x) * np.sin(np.pi / 2) * np.cos(0)) / \
            (np.sin(np.pi / 2) * np.sin(np.pi / 2 + angle_x) + np.cos(np.pi / 2 + angle_x) * np.cos(np.pi / 2) * np.cos(0)))

        ly = abs((np.sin(np.pi / 2 + angle_y) * np.cos(np.pi / 2) - np.cos(np.pi / 2 + angle_y) * np.sin(np.pi / 2) * np.cos(0)) / \
             (np.sin(np.pi / 2) * np.sin(np.pi / 2 + angle_y) + np.cos(np.pi / 2 + angle_y) * np.cos(np.pi / 2) * np.cos(0)))

        # Применение стереографической проекции для координат (A, z)
        x_proj = np.tan((np.pi/2 - z_above) / 2) * np.cos(A_above)  # ksi
        y_proj = np.tan((np.pi/2 - z_above) / 2) * np.sin(A_above)  # eta

        # Звезды, которые находятся в поле зрения
        in_fov = (x_proj >= -lx) & (x_proj <= lx) & (y_proj >= -ly) & (y_proj <= ly)
        x_fov = x_proj[in_fov]
        y_fov = y_proj[in_fov]
        magnitude_fov = magnitude_visible[in_fov]

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

        plt.show()