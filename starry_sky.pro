QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += src/Headers

# Источники
SOURCES += \
    src/Sources/GaussianBlur.cpp \
    src/Sources/LightPollution.cpp \
    src/Sources/StarCatalog.cpp \
    src/Sources/StarMapWidget.cpp \
    src/Sources/main.cpp \
    src/Sources/mainwindow.cpp

# Заголовки
HEADERS += \
    src/Headers/GaussianBlur.h \
    src/Headers/LightPollution.h \
    src/Headers/StarCatalog.h \
    src/Headers/StarMapWidget.h \
    src/Headers/mainwindow.h

# Форма для UI (если используется)
FORMS += \
    src/Forms/mainwindow.ui

# Каталог данных
DISTFILES += \
    src/data/Catalogue.csv \
    src/data/Catalogue_clean.csv \
    src/research/data_cleaning.ipynb \
    src/research/star_sky_image.py \
    starry_sky.pro.user

# Устанавливаем пути для QNX/Unix
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
