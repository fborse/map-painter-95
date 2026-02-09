QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ColorGradientWidget.cpp \
    ColorHueWidget.cpp \
    ColorPaletteWidget.cpp \
    ColorSaturationValueWidget.cpp \
    ColorSelectionWidget.cpp \
    EditorWidget.cpp \
    MapEditorWidget.cpp \
    MapPainterWidget.cpp \
    TilesetView.cpp \
    main.cpp \
    MainWindow.cpp

HEADERS += \
    ColorGradientWidget.hpp \
    ColorHueWidget.hpp \
    ColorPaletteWidget.hpp \
    ColorSaturationValueWidget.hpp \
    ColorSelectionWidget.hpp \
    EditorWidget.hpp \
    MainWindow.hpp \
    MapEditorWidget.hpp \
    MapPainterWidget.hpp \
    TilesetView.hpp

FORMS += \
    MainWindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
