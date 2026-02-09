#pragma once

#include "EditorWidget.hpp"

class MapPainterWidget final: public EditorWidget
{
    Q_OBJECT
public:
    explicit MapPainterWidget(QWidget *parent = nullptr);
    virtual ~MapPainterWidget() final override = default;

    QColor getDrawColor() const { return draw_color; }

public slots:
    void setDrawColor(const QColor color) { draw_color = color; }
    void setShowGrid(const bool yes) { show_grid = yes; update(); }

private:
    QColor draw_color;
    bool show_grid;

    QPoint mouse_cursor;
    std::optional<QPoint> click_origin;

    void paintEvent(QPaintEvent *) override;
};
