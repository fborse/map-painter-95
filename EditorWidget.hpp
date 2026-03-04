#pragma once

#include <QWidget>
#include <QUndoStack>

#include "Types.hpp"

class EditorWidget: public QWidget
{
    Q_OBJECT
public:
    explicit EditorWidget(QWidget *parent = nullptr);
    virtual ~EditorWidget() = default;

    void setUndoStackPointer(QSharedPointer<QUndoStack> ptr) { undo_stack = ptr; }
    void setSimpleTilesOrderPointer(QSharedPointer<Names> ptr) { simple_tiles_order = ptr; }
    void setAutoTilesOrderPointer(QSharedPointer<Names> ptr) { autotiles_order = ptr; }
    void setSimpleTilesPointer(QSharedPointer<SimpleTiles> ptr) { simple_tiles = ptr; }
    void setSelectedTilesPointer(QSharedPointer<SelectedTiles> ptr) { selected_tiles = ptr; }
    void setMapLayersPointer(QSharedPointer<MapLayers> ptr) { map_layers = ptr; }

    int getTilesize() const { return tilesize; }

    virtual void resize();

public slots:
    void setGridAspect(const QSize aspect) { grid_aspect = aspect; resize(); }
    void setTilesize(const int size) { tilesize = size; resize(); }
    virtual void setZoom(const double z) { zoom = z; resize(); }
    void setCurrentLayer(const int layer);
    void setCurrentFrame(const int frame);

signals:
    void zoomSet(const double zoom);
    void scrolledAt(const QPoint p);

protected:
    QSize grid_aspect;
    int tilesize;

    double zoom;
    int current_layer;
    int current_frame;

    QSharedPointer<QUndoStack> undo_stack;
    QSharedPointer<Names> simple_tiles_order, autotiles_order;
    QSharedPointer<SimpleTiles> simple_tiles;
    QSharedPointer<SelectedTiles> selected_tiles;
    QSharedPointer<MapLayers> map_layers;

    QRect getWidgetRect() const { return {QPoint(0, 0), grid_aspect * tilesize * zoom}; }
//  TODO: find a better name (widget coordinates to grid coordinate rect)
    QRect asLocalRect(const QPoint &p1, const QPoint &p2) const;

    void paintBackground(QPainter &painter) const;
    QImage getPaintedLayer(const int layer) const;
    void paintGrid(QPainter &painter) const;

    void wheelEvent(QWheelEvent *event) override;
};
