#pragma once

#include "EditorWidget.hpp"

enum DragMode {
    SELECTION_MODE = 0,
    MOVE_MODE = 1,
    SWAP_MODE = 2
};

class TilesetView final: public EditorWidget
{
    Q_OBJECT
public:
    explicit TilesetView(QWidget *parent = nullptr);
    virtual ~TilesetView() final override = default;

    int getNumberOfColumns() const { return n_columns; }

    void resize() final override;

public slots:
    void setNumberOfColumns(const int n) { n_columns = n; resize(); }
    void setDragMode(const int index);

private:
    int n_columns;
    DragMode drag_mode;

    QPoint mouse_cursor;
    std::optional<QPoint> click_origin;

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
