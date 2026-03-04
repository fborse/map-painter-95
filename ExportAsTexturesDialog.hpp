#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ExportAsTexturesDialog; }
QT_END_NAMESPACE

#include "Tileset.hpp"

class ExportAsTexturesDialog: public QDialog
{
    Q_OBJECT
public:
    explicit ExportAsTexturesDialog(const int tilesize, QWidget *parent = nullptr);
    ~ExportAsTexturesDialog();

    void setSimpleTilesPointer(QWeakPointer<SimpleTiles> ptr) { simple_tiles = ptr; }
    void setMapLayersPointer(QWeakPointer<MapLayers> ptr);

    QString getDirectory() const;
    QString getPattern() const;

    int getNumberOfLayers() const;
    int getNumberOfFrames() const;

public slots:
    void onAccept();
    void setCurrentLayer(const int layer);
    void setCurrentFrame(const int frame);

private:
    Ui::ExportAsTexturesDialog *ui;

    int tilesize;
    int current_layer;
    int current_frame;
    QVector<QVector<QImage>> drawn_textures;

    QWeakPointer<SimpleTiles> simple_tiles;
    QWeakPointer<MapLayers> map_layers;

    void redrawTextures();
//  this one is only called when setting map_layers
    void updateLayersComboBox();
    void updateFramesComboBox();
    void updateDisplayedTexture();
};
