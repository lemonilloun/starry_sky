QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Источники
SOURCES += \
    GaussianBlur.cpp \
    LightPollution.cpp \
    StarCatalog.cpp \
    StarMapWidget.cpp \
    main.cpp \
    mainwindow.cpp

# Заголовки
HEADERS += \
    GaussianBlur.h \
    LightPollution.h \
    StarCatalog.h \
    StarMapWidget.h \
    StarMapWidget.h \  # Добавляем заголовок для виджета карты звезд
    mainwindow.h

# Форма для UI (если используется)
FORMS += \
    mainwindow.ui

# Каталог данных
DISTFILES += \
    Catalogue.csv \
    Catalogue_clean.csv

# Устанавливаем пути для QNX/Unix
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
