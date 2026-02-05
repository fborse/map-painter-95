#include "MapPainterWidget.hpp"

#include <QPainter>

MapPainterWidget::MapPainterWidget(QWidget *parent):
    EditorWidget(parent)
{}

void MapPainterWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    paintBackground(painter);
//    paintGrid(painter);
}
