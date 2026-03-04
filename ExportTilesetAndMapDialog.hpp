#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ExportTilesetAndMapDialog; }
QT_END_NAMESPACE

#include "Tileset.hpp"

class ExportTilesetAndMapDialog: public QDialog
{
    Q_OBJECT
public:
    explicit ExportTilesetAndMapDialog(const int tilesize, QWidget *parent = nullptr);
    ~ExportTilesetAndMapDialog();

    void setTilesOrderPointer(QWeakPointer<Names> ptr) { tiles_order = ptr; redrawTileset(); }
    void setSimpleTilesPointer(QWeakPointer<SimpleTiles> ptr) { simple_tiles = ptr; redrawTileset(); }
    void setMapLayersPointer(QWeakPointer<MapLayers> ptr) { map_layers = ptr; }

    QString getTilesetPath() const;
    QString getMapPath() const;

    int getNumberOfColumns() const;

public slots:
    void onAccept();
    void redrawTileset();

private:
    Ui::ExportTilesetAndMapDialog *ui;

    int tilesize;

    QHash<QString, QVector<QPoint>> tile_coordinates;
    QImage drawn_tileset;

    QWeakPointer<Names> tiles_order;
    QWeakPointer<SimpleTiles> simple_tiles;
    QWeakPointer<MapLayers> map_layers;
};
