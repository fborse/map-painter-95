#include "MapEditorWidget.hpp"

#include <QPainter>

MapEditorWidget::MapEditorWidget(QWidget *parent):
    EditorWidget(parent)
{
    resize();
}

void MapEditorWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    paintBackground(painter);
    paintGrid(painter);
}
