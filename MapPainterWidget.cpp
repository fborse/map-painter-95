#include "MapPainterWidget.hpp"

#include <QPainter>

MapPainterWidget::MapPainterWidget(QWidget *parent):
    EditorWidget(parent),
    mouse_cursor{}, click_origin{}
{}

void MapPainterWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    paintBackground(painter);

    painter.scale(zoom, zoom);
    paintLayers(painter);
    painter.resetTransform();

//    paintGrid(painter);
}
