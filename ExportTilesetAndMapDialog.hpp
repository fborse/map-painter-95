#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ExportTilesetAndMapDialog; }
QT_END_NAMESPACE

#include "Types.hpp"

class ExportTilesetAndMapDialog: public QDialog
{
    Q_OBJECT
public:
    explicit ExportTilesetAndMapDialog(const int tilesize, QWidget *parent = nullptr);
    ~ExportTilesetAndMapDialog();

    void setSimpleTilesOrderPointer(QWeakPointer<Names> ptr) { simple_tiles_order = ptr; redrawTileset(); }
    void setAutoTilesOrderPointer(QWeakPointer<Names> ptr) { autotiles_order = ptr; redrawTileset(); }
    void setSimpleTilesPointer(QWeakPointer<SimpleTiles> ptr) { simple_tiles = ptr; redrawTileset(); }
    void setAutoTilesPointer(QWeakPointer<AutoTiles> ptr) { autotiles = ptr; redrawTileset(); }
    void setMapLayersPointer(QWeakPointer<MapLayers> ptr) { map_layers = ptr; redrawTileset(); }

    QString getTilesetPath() const;
    QString getMapPath() const;

    int getNumberOfColumns() const;

public slots:
    void onAccept();
    void redrawTileset();

private:
    Ui::ExportTilesetAndMapDialog *ui;

    int tilesize;

    QHash<TileReference, QVector<QPoint>> tile_coordinates;
    QImage drawn_tileset;

    QWeakPointer<Names> simple_tiles_order, autotiles_order;
    QWeakPointer<SimpleTiles> simple_tiles;
    QWeakPointer<AutoTiles> autotiles;
    QWeakPointer<MapLayers> map_layers;
};
