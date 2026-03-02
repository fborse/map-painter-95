#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ExportAsTexturesDialog; }
QT_END_NAMESPACE

using Tile = QVector<QImage>;
using Tileset = QHash<QString, Tile>;

using TileReference = QString;
using MapLayer = QVector<QVector<TileReference>>;
using MapLayers = QVector<MapLayer>;

class ExportAsTexturesDialog: public QDialog
{
    Q_OBJECT
public:
    explicit ExportAsTexturesDialog(const int tilesize, QWidget *parent = nullptr);
    ~ExportAsTexturesDialog();

    void setTilesetPointer(QWeakPointer<Tileset> ptr) { tileset = ptr; }
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

    QWeakPointer<Tileset> tileset;
    QWeakPointer<MapLayers> map_layers;

    void redrawTextures();
//  this one is only called when setting map_layers
    void updateLayersComboBox();
    void updateFramesComboBox();
    void updateDisplayedTexture();
};
