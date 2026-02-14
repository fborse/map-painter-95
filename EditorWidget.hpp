#pragma once

#include <QWidget>
#include <QUndoStack>

using Names = QVector<QString>;
using Tileset = QHash<QString, QImage>;
using SelectedTiles = QVector<QVector<QString>>;

using TileReference = QString;
using MapLayer = QVector<QVector<TileReference>>;

class EditorWidget: public QWidget
{
    Q_OBJECT
public:
    explicit EditorWidget(QWidget *parent = nullptr);
    virtual ~EditorWidget() = default;

    void setUndoStackPointer(QSharedPointer<QUndoStack> ptr) { undo_stack = ptr; }
    void setTilesOrderPointer(QSharedPointer<Names> ptr) { tiles_order = ptr; }
    void setTilesetPointer(QSharedPointer<Tileset> ptr) { tileset = ptr; }
    void setSelectedTilesPointer(QSharedPointer<SelectedTiles> ptr) { selected_tiles = ptr; }
    void setMapLayersPointer(QSharedPointer<MapLayer> ptr) { map_layers = ptr; }

    int getTilesize() const { return tilesize; }

    virtual void resize();

public slots:
    void setGridAspect(const QSize aspect) { grid_aspect = aspect; resize(); }
    void setTilesize(const int size) { tilesize = size; resize(); }
    virtual void setZoom(const double z) { zoom = z; resize(); }

signals:
    void zoomSet(const double zoom);

protected:
    QSize grid_aspect;
    int tilesize;
    double zoom;

    QSharedPointer<QUndoStack> undo_stack;
    QSharedPointer<Names> tiles_order;
    QSharedPointer<Tileset> tileset;
    QSharedPointer<SelectedTiles> selected_tiles;
    QSharedPointer<MapLayer> map_layers;

    QRect getWidgetRect() const { return {QPoint(0, 0), grid_aspect * tilesize * zoom}; }
//  TODO: find a better name (widget coordinates to grid coordinate rect)
    QRect asLocalRect(const QPoint &p1, const QPoint &p2) const;

    void paintBackground(QPainter &painter) const;
    QImage getPaintedLayer() const;
    void paintGrid(QPainter &painter) const;

    void wheelEvent(QWheelEvent *event) override;
};
