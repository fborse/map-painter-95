#pragma once

#include "EditorWidget.hpp"

enum DrawTool
{
    PEN = 0,
    LINE = 1,
    BRUSH = 2,
    SHAPE = 3,
    FILL = 4,
    ERASER = 5,
    SHADER = 6,
    PIPETTE = 7,
    SELECTION = 8
};

class MapPainterWidget final: public EditorWidget
{
    Q_OBJECT
public:
    explicit MapPainterWidget(QWidget *parent = nullptr);
    virtual ~MapPainterWidget() final override = default;

    QColor getDrawColor() const { return draw_color; }

public slots:
    void setShowGrid(const bool yes) { show_grid = yes; update(); }
    void setDrawTool(const int index);

    void setDrawColor(const QColor color) { draw_color = color; }
    void setPenSize(const int size) { pen_size = size; }

signals:
    void colorChanged(const QColor color);

private:
    bool show_grid;
    DrawTool draw_tool;

    QColor draw_color;
    int pen_size;

    QPoint mouse_cursor;
//  right click is really just a desktop thing => names accordingly
    std::optional<QPoint> click_origin;
    bool right_click;

    QVector<QPoint> drag_points;

    void paintCursor(QPainter &painter) const;
    void paintEvent(QPaintEvent *) override;

    QColor getColorAt(const QPoint &p) const;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};
