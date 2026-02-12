#include "BrushDisplayWidget.hpp"

#include <QPainter>
#include <QMouseEvent>

#include "BrushEditorDialog.hpp"

BrushDisplayWidget::BrushDisplayWidget(QWidget *parent):
    QWidget(parent), brush_pixels{}
{
    resize();
}

void BrushDisplayWidget::resize()
{
    setFixedSize(brush_pixels.size() * grid_size);
    update();
}

void BrushDisplayWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    const int unit = grid_size;

    painter.scale(unit, unit);
    painter.drawImage(0, 0, brush_pixels);
    painter.resetTransform();

    const QColor white64 = {255, 255, 255, 64};
    for (int j = 0; j < brush_pixels.height(); ++j)
        painter.fillRect(0, j * unit, width(), 1, white64);
    painter.fillRect(0, height() - 1, width(), 1, white64);
    for (int i = 0; i < brush_pixels.width(); ++i)
        painter.fillRect(i * unit, 0, 1, height(), white64);
    painter.fillRect(width() - 1, 0, 1, height(), white64);
}

void BrushDisplayWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (rect().contains(event->pos()))
    {
        BrushEditorDialog dialog(brush_pixels, this);
        if (dialog.exec() == QDialog::Accepted)
            emit brushChanged(dialog.getBrushPixels());
    }
}
