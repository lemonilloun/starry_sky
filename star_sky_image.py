import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

class star_sky:
    def __init__(self, RA, DEC,fov=[58, 47], magnitude_limit=3.8, pix=16/1024):
        # Загрузка каталога звёзд (предполагаем, что это CSV файл)
        catalogue = pd.read_csv('Catalogue.csv')  # CSV-файл каталога звёзд

        self.fov = fov

        # координаты точки зенита в экваториальных координатах
        self.RA_observer = RA
        self.DEC_observer = DEC
    
        resolution = [1280, 1038]
        self.ra = np.radians(catalogue['RA'])  # Прямое восхождение в радианах
        self.dec = np.radians(catalogue['Dec'])  # Склонение в радианах
        self.magnitude = catalogue['Mag']  # Звёздная величина
        self.magnitude_limit = magnitude_limit # Предельная звёздная величина
        self.matrix_size = np.array(resolution) * pix  # Размер матрицы камеры в


    @staticmethod
    def julian_date(dt):
        # Вспомогательная функция для вычисления юлианской даты
        a = (14 - dt.month) // 12
        y = dt.year + 4800 - a
        m = dt.month + 12 * a - 3
        jdn = dt.day + ((153 * m + 2) // 5) + 365 * y + y // 4 - y // 100 + y // 400 - 32045
        jd = jdn + (dt.hour - 12) / 24 + dt.minute / 1440 + dt.second / 86400
        return jd

    #TODO: переписать фукнцию перехода из экваториальных координат в горизонтальные с заданной точкой зенита
    def transform(self):
        # Фильтрация звёзд по предельной звёздной величине
        visible_stars = self.magnitude <= self.magnitude_limit
        ra_visible = self.ra[visible_stars]  # alpha
        dec_visible = self.dec[visible_stars]  # delta
        magnitude_visible = self.magnitude[visible_stars]

        print(f'количество звезд, которые не больше заданной зв: {len(ra_visible)} ')

        # Часовой угол (H)
        H = ra_visible - self.RA_observer  # аналог t в MATLAB-коде

        # Зенитное расстояние (z)
        z = np.arccos(np.sin(dec_visible) * np.sin(self.DEC_observer) + 
                  np.cos(dec_visible) * np.cos(self.DEC_observer) * np.cos(H))
    
        # Азимут A1 и A2
        A1 = np.arcsin(np.cos(dec_visible) * np.sin(H) / np.sin(z))
        A2 = np.arccos((-np.sin(dec_visible) * np.cos(self.DEC_observer) + 
                    np.cos(dec_visible) * np.sin(self.DEC_observer) * np.cos(H)) / np.sin(z))

        # Логика для определения азимута A
        A = np.zeros_like(A1)  # Инициализация массива для азимута
        A[A1 > 0] = A2[A1 > 0]
        A[A1 < 0] = 2 * np.pi - A2[A1 < 0]
        A[np.abs(A1) <= 1e-6] = 0  # Если A1 близко к нулю, устанавливаем A в 0

        # Высота светила h (в радианах)
        h = np.pi / 2 - z

        return A,magnitude_visible, h

    def filtering_stars(self, A, z, magnitude_visible):
        # Фильтрация звёзд, которые находятся выше горизонта
        above_horizon = z >= 0
        z_above = z[above_horizon]
        A_above = A[above_horizon]
        magnitude_above = magnitude_visible[above_horizon]

        # Углы поля зрения (в радианах)
        angle_x = np.radians(self.fov[0] / 2)  # Половина углового поля зрения по горизонтали
        angle_y = np.radians(self.fov[1] / 2)  # Половина углового поля зрения по вертикали

        # Нахождение границ ksi (lx) и eta (ly) в кадре
        lx = abs((np.sin(np.pi / 2 + angle_x) * np.cos(np.pi / 2) - np.cos(np.pi / 2 + angle_x) * np.sin(np.pi / 2) * np.cos(0)) / 
             (np.sin(np.pi / 2) * np.sin(np.pi / 2 + angle_x) + np.cos(np.pi / 2 + angle_x) * np.cos(np.pi / 2) * np.cos(0)))

        ly = abs((np.sin(np.pi / 2 + angle_y) * np.cos(np.pi / 2) - np.cos(np.pi / 2 + angle_y) * np.sin(np.pi / 2) * np.cos(0)) / 
             (np.sin(np.pi / 2) * np.sin(np.pi / 2 + angle_y) + np.cos(np.pi / 2 + angle_y) * np.cos(np.pi / 2) * np.cos(0)))

        # Применение стереографической проекции для координат (A_above, z_above)
        x_proj = np.tan(z_above / 2) * np.cos(A_above)  # координата ksi
        y_proj = np.tan(z_above / 2) * np.sin(A_above)  # координата eta

        # Фильтрация звезд, попадающих в поле зрения
        in_fov = (x_proj >= -lx) & (x_proj <= lx) & (y_proj >= -ly) & (y_proj <= ly)  # маска для звёзд в поле зрения

        # Выборка звёзд, которые находятся в пределах рамки изображения
        x_fov = x_proj[in_fov]
        y_fov = y_proj[in_fov]
        magnitude_fov = magnitude_above[in_fov]

        # Отладочная информация о количестве звёзд, попавших в объектив
        print(f'Звезды, которые попадают в объектив: {len(x_fov)}')

        return (x_fov, y_fov, magnitude_fov), (lx, ly)


    def get_image(self, x_fov, y_fov, bords, mag = None):
        lx, ly = bords
        # Настройка графика
        plt.figure(figsize=(8, 8),facecolor='black')
        plt.scatter(x_fov, y_fov, s=1, color='white')

        # Настройки графика
        plt.xlim([-lx, lx])
        plt.ylim([-ly, ly])
        plt.gca().set_facecolor('black')
        plt.gca().set_aspect('equal')
        #plt.axis('off')

        # Показать график
        plt.show()