#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ExportAsTexturesDialog; }
QT_END_NAMESPACE

using Tileset = QHash<QString, QImage>;

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
    void setMapLayersPointer(QWeakPointer<MapLayers> ptr) { map_layers = ptr; redrawLayers(); }

    QString getDirectory() const;
    QString getPattern() const;

    int getNumberOfLayers() const;
//  TODO: Implement in a distant future
    int getNumberOfFrames() const;

public slots:
    void onAccept();
//  TODO:
    void setCurrentLayer(const int layer) { current_layer = layer; updateLayerLabel(); }

private:
    Ui::ExportAsTexturesDialog *ui;

    int tilesize;
    int current_layer;
    QVector<QImage> drawn_layers;

    QWeakPointer<Tileset> tileset;
    QWeakPointer<MapLayers> map_layers;

    void redrawLayers();
    void updateLayerLabel();
};
