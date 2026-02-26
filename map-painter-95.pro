QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    AddRectDialog.cpp \
    BrushDisplayWidget.cpp \
    BrushEditorDialog.cpp \
    BrushEditorWidget.cpp \
    ColorGradientWidget.cpp \
    ColorHueWidget.cpp \
    ColorPaletteWidget.cpp \
    ColorSaturationValueWidget.cpp \
    ColorSelectionWidget.cpp \
    EditorWidget.cpp \
    ExportAsTexturesDialog.cpp \
    ExportTilesetAndMapDialog.cpp \
    ImportTilesInBulkDialog.cpp \
    MapEditorWidget.cpp \
    MapPainterWidget.cpp \
    NewMapDialog.cpp \
    ResizeMapDialog.cpp \
    ScaleSelectionDialog.cpp \
    TilesetViewWidget.cpp \
    main.cpp \
    MainWindow.cpp

HEADERS += \
    AddRectDialog.hpp \
    BrushDisplayWidget.hpp \
    BrushEditorDialog.hpp \
    BrushEditorWidget.hpp \
    ColorGradientWidget.hpp \
    ColorHueWidget.hpp \
    ColorPaletteWidget.hpp \
    ColorSaturationValueWidget.hpp \
    ColorSelectionWidget.hpp \
    EditorWidget.hpp \
    ExportAsTexturesDialog.hpp \
    ExportTilesetAndMapDialog.hpp \
    ImportTilesInBulkDialog.hpp \
    MainWindow.hpp \
    MapEditorWidget.hpp \
    MapPainterWidget.hpp \
    NewMapDialog.hpp \
    ResizeMapDialog.hpp \
    ScaleSelectionDialog.hpp \
    TilesetViewWidget.hpp

FORMS += \
    AddRectDialog.ui \
    BrushEditorDialog.ui \
    ExportAsTexturesDialog.ui \
    ExportTilesetAndMapDialog.ui \
    ImportTilesInBulkDialog.ui \
    MainWindow.ui \
    NewMapDialog.ui \
    ResizeMapDialog.ui \
    ScaleSelectionDialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
