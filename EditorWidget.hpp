#pragma once

#include <QWidget>
#include <QUndoStack>

using Names = QVector<QString>;
using Tileset = QHash<QString, QImage>;
using SelectedTiles = QVector<QVector<QString>>;

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

    virtual void resize();

public slots:
    void setTilesize(const int size) { tilesize = size; resize(); }
    void setZoom(const double z) { zoom = z; resize(); }

protected:
    QSize grid_aspect;
    int tilesize;
    double zoom;

    QSharedPointer<QUndoStack> undo_stack;
    QSharedPointer<Names> tiles_order;
    QSharedPointer<Tileset> tileset;
    QSharedPointer<SelectedTiles> selected_tiles;

    QRect getWidgetRect() const { return {QPoint(0, 0), grid_aspect * tilesize * zoom}; }
//  TODO: find a better name (widget coordinates to grid coordinate rect)
    QRect asLocalRect(const QPoint &p1, const QPoint &p2) const;

    void paintBackground(QPainter &painter) const;
    void paintGrid(QPainter &painter) const;
};
