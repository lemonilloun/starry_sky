QT += core gui widgets openglwidgets

CONFIG += c++17

INCLUDEPATH += src/Headers

SOURCES += \
    src/Sources/CelestialBodyTypes.cpp \
    src/Sources/GaussianBlur.cpp \
    src/Sources/LightPollution.cpp \
    src/Sources/MoonEphemeris.cpp \
    src/Sources/MyGLWidget.cpp \
    src/Sources/PlanetEphemeris.cpp \
    src/Sources/SettingsDialog.cpp \
    src/Sources/SunEphemeris.cpp \
    src/Sources/StarCatalog.cpp \
    src/Sources/StarMapWidget.cpp \
    src/Sources/main.cpp \
    src/Sources/mainwindow.cpp

HEADERS += \
    src/Headers/CelestialBodyTypes.h \
    src/Headers/GaussianBlur.h \
    src/Headers/LightPollution.h \
    src/Headers/MoonEphemeris.h \
    src/Headers/MyGLWidget.h \
    src/Headers/PlanetEphemeris.h \
    src/Headers/SettingsDialog.h \
    src/Headers/SunEphemeris.h \
    src/Headers/StarCatalog.h \
    src/Headers/StarMapWidget.h \
    src/Headers/mainwindow.h

FORMS += \
    src/Forms/mainwindow.ui

DISTFILES += \
    src/data/Catalogue.csv \
    src/data/Catalogue_clean.csv \
    src/research/data_cleaning.ipynb \
    src/research/star_sky_image.py \
    starry_sky.pro.user

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
