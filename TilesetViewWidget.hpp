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

    void addTiles(const QVector<SimpleTile> &tiles, const bool undoable);
    void removeTiles(const QVector<TileReference> &tiles);

    void addFrames(const QHash<int, QImage> &frames);
    void removeFrames(const QVector<int> &indexes);

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

    std::optional<QPoint> toIJ(const int idx) const;
    std::optional<int> toIndex(const QPoint &ij) const;

    void paintTileset(QPainter &painter);
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
