#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ExportTilesetAndMapDialog; }
QT_END_NAMESPACE

using Names = QVector<QString>;
using Tileset = QHash<QString, QImage>;

using TileReference = QString;
using MapLayer = QVector<QVector<TileReference>>;

class ExportTilesetAndMapDialog: public QDialog
{
    Q_OBJECT
public:
    explicit ExportTilesetAndMapDialog(const int tilesize, QWidget *parent = nullptr);
    ~ExportTilesetAndMapDialog();

    void setTilesOrderPointer(QWeakPointer<Names> ptr) { tiles_order = ptr; redrawTileset(); }
    void setTilesetPointer(QWeakPointer<Tileset> ptr) { tileset = ptr; redrawTileset(); }
    void setMapLayersPointer(QWeakPointer<MapLayer> ptr) { map_layers = ptr; }

    QString getTilesetPath() const;
    QString getMapPath() const;

    int getNumberOfColumns() const;

public slots:
    void onAccept();
    void redrawTileset();

private:
    Ui::ExportTilesetAndMapDialog *ui;

    int tilesize;

    QHash<QString, QPoint> tile_coordinates;
    QImage drawn_tileset;

    QWeakPointer<Names> tiles_order;
    QWeakPointer<Tileset> tileset;
    QWeakPointer<MapLayer> map_layers;
};
