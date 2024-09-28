import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

class star_sky:
    def __init__(self, RA, DEC, longitude=0, latitude=0, observation_time=0, fov=[58, 47], magnitude_limit=3.8, pix=16/1024):
        # Загрузка каталога звёзд (предполагаем, что это CSV файл)
        catalogue = pd.read_csv('Catalogue.csv')  # CSV-файл каталога звёзд

        self.fov = fov

        # координаты точки зенита в экваториальных координатах
        self.RA = RA
        self.DEC = DEC
    
        # Извлечение данных: RA, Dec и видимая величина звёзд
        self.longitude = np.radians(longitude)
        self.latitude = np.radians(latitude)
        self.observation_time = observation_time
        resolution = [1280, 1038]
        self.ra = np.radians(catalogue['RA'])  # Прямое восхождение в радианах
        self.dec = np.radians(catalogue['Dec'])  # Склонение в радианах
        self.magnitude = catalogue['Mag']  # Звёздная величина
        self.magnitude_limit = 3.8  # Предельная звёздная величина
        self.matrix_size = np.array(resolution) * pix  # Размер матрицы камеры в
    
    def calculate_lst(self):
        # Вычисление юлианской даты
        jd = self.julian_date(self.observation_time)
        
        # Вычисление числа дней с эпохи J2000.0
        d = jd - 2451545.0
        
        # Вычисление среднего звездного времени в Гринвиче в полночь (в градусах)
        gmst = 280.46061837 + 360.98564736629 * d
        
        # Нормализация GMST в диапазоне [0, 360)
        gmst = gmst % 360
        
        # Перевод GMST в часы
        gmst_hours = gmst / 15
        
        # Вычисление местного звездного времени
        lst_hours = (gmst_hours + self.longitude * 180 / np.pi / 15) % 24
        
        # Перевод LST в радианы
        lst_radians = lst_hours * 15 * np.pi / 180
        
        return lst_radians

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
    def transform(self, lat=np.radians(55.0) , lst=np.radians(11.0)):
       # Параметры наблюдателя (координаты зенита)
        lat = np.radians(55.0)  # широта Москвы в радианах
        lst = np.radians(11.0)  # местное звёздное время в радианах (примерное)

        # Фильтрация звёзд по предельной звёздной величине
        visible_stars = self.magnitude <= self.magnitude_limit
        ra_visible = self.ra[visible_stars]
        dec_visible = self.dec[visible_stars]
        magnitude_visible = self.magnitude[visible_stars]

        # Часовой угол (H) для каждой звезды
        H = lst - ra_visible

        #Формула для зенитного расстояния z
        cos_h = np.sin(dec_visible) * np.sin(lat) + np.cos(dec_visible) * np.cos(lat) * np.cos(H)
        cos_h = np.clip(cos_h, -1.0, 1.0)  # Ограничиваем значения для предотвращения ошибки arccos
        h = np.arccos(cos_h)  # зенитное расстояние в радианах

        # Зенитное расстояние (z) = 90° - высота
        z = np.pi / 2 - h

        # Формула для синуса азимута A
        sin_h_safe = np.where(np.abs(np.sin(h)) < 1e-10, 1e-10, np.sin(h))  # Избегаем деления на ноль
        sin_A = np.cos(dec_visible) * np.sin(H) / sin_h_safe

        # Формула для косинуса азимута A
        cos_A = (-np.sin(dec_visible) + np.cos(dec_visible) * np.sin(lat) * np.cos(H)) / sin_h_safe

        # Азимут
        A = np.arctan2(sin_A, cos_A)  # вычисление азимута через atan2

        return (A,z,magnitude_visible, h)

    def filtering_stars(self, A, z, magnitude_visible, h):
        # Фильтрация звёзд, которые находятся выше горизонта
        above_horizon = h >= 0
        z_above = z[above_horizon]
        A_above = A[above_horizon]
        magnitude_above = magnitude_visible[above_horizon]

        # Углы поля зрения (в радианах)
        angle_x = np.radians(self.fov[0] / 2)  # Половина углового поля зрения по горизонтали
        angle_y = np.radians(self.fov[1] / 2)  # Половина углового поля зрения по вертикали

        # Нахождение границ ksi (lx) и eta (ly)
        lx = abs((np.sin(np.pi / 2 + angle_x) * np.cos(np.pi / 2) - np.cos(np.pi / 2 + angle_x) * np.sin(np.pi / 2) * np.cos(0)) / \
            (np.sin(np.pi / 2) * np.sin(np.pi / 2 + angle_x) + np.cos(np.pi / 2 + angle_x) * np.cos(np.pi / 2) * np.cos(0)))

        ly = abs((np.sin(np.pi / 2 + angle_y) * np.cos(np.pi / 2) - np.cos(np.pi / 2 + angle_y) * np.sin(np.pi / 2) * np.cos(0)) / \
             (np.sin(np.pi / 2) * np.sin(np.pi / 2 + angle_y) + np.cos(np.pi / 2 + angle_y) * np.cos(np.pi / 2) * np.cos(0)))

        # Применение стереографической проекции для координат (A, z)
        x_proj = np.tan(z_above / 2) * np.cos(A_above)  # ksi
        y_proj = np.tan(z_above / 2) * np.sin(A_above) # eta

        # Звезды, которые находятся в поле зрения
        in_fov = (x_proj >= -lx) & (x_proj <= lx) & (y_proj >= -ly) & (y_proj <= ly) # маска для массива 
        x_fov = np.degrees(x_proj[in_fov])
        y_fov = np.degrees(y_proj[in_fov])
        magnitude_fov = magnitude_above[in_fov]

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