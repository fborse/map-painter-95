#pragma once

#include "EditorWidget.hpp"

enum DragMode {
    SELECTION_MODE = 0,
    MOVE_MODE = 1,
    SWAP_MODE = 2
};

class TilesetViewWidget final: public EditorWidget
{
    Q_OBJECT
public:
    explicit TilesetViewWidget(QWidget *parent = nullptr);
    virtual ~TilesetViewWidget() final override = default;

    int getNumberOfColumns() const { return n_columns; }

    void resize() final override;

//  these two operations are meant to be included into a macro
//  => 1 command push on undo_stack
    void addSimpleTile(const SimpleTile &simple_tile);
    void addAutoTile(const AutoTile &autotile);
    void removeTiles(const QVector<TileReference> &tiles);

//  these operations are atomic (1 command push on undo_stack)
//  in order to be combined when the user uses the frame order editor
    void addFrame(const int index, const QImage &frame);
    void addFrame(const int index, const AutoTile::Frame &frame);
    void removeFrame(const int index);

public slots:
    void setNumberOfColumns(const int n) { n_columns = n; resize(); }
    void setDragMode(const int index);

signals:
    void tilesRemoved();
    void selectedChanged();

private:
    int n_columns;
    DragMode drag_mode;

    QPoint mouse_cursor;
//  right click is really just a desktop thing => names accordingly
    std::optional<QPoint> click_origin, right_click_origin;

    std::optional<TileReference> toRef(const QPoint &ij) const;

    void paintAutoTiles(QPainter &painter);
    void paintSimpleTiles(QPainter &painter);
    void paintSelectionCursors(QPainter &painter);
    void paintCursor(QPainter &painter);
    void paintSelectionRect(QPainter &painter);

    void paintEvent(QPaintEvent *) override;

    void handleTilesSelected();
    void handleTileModifications();

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};
