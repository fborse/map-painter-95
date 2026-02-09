#include "MapPainterWidget.hpp"

#include <QPainter>

MapPainterWidget::MapPainterWidget(QWidget *parent):
    EditorWidget(parent),
    draw_color{Qt::red}, show_grid{false},
    mouse_cursor{}, click_origin{}
{}

void MapPainterWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    paintBackground(painter);

    painter.scale(zoom, zoom);
    paintLayers(painter);
    painter.resetTransform();

    if (show_grid)
        paintGrid(painter);
}
