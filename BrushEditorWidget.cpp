#include "BrushEditorWidget.hpp"

#include <QPainter>
#include <QMouseEvent>

BrushEditorWidget::BrushEditorWidget(QWidget *parent):
    QWidget(parent),
    zoom{8}, brush_pixels{}
{
    resize();
}

void BrushEditorWidget::toggleAt(const QPoint &p)
{
    if (brush_pixels.pixelColor(p) == Qt::black)
        brush_pixels.setPixelColor(p, Qt::white);
    else
        brush_pixels.setPixelColor(p, Qt::black);
}

void BrushEditorWidget::resizePixels(const QSize &size)
{
    QImage pixels(size, QImage::Format_ARGB32_Premultiplied);
    pixels.fill(Qt::black);

    QPainter painter(&pixels);
    painter.drawImage(0, 0, brush_pixels);

    brush_pixels = pixels;
    resize();
}

void BrushEditorWidget::resize()
{
    setFixedSize(brush_pixels.size() * grid_size * zoom);
    update();
}

void BrushEditorWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    const int unit = grid_size * zoom;

    const auto &[w, h] = brush_pixels.size();
    for (int i = 0; i < w; ++i)
        for (int j = 0; j < h; ++j)
            painter.fillRect(i * unit, j * unit, unit, unit, brush_pixels.pixelColor(i, j));

    const QColor white128 = {255, 255, 255, 128};
    for (int j = 0; j < h; ++j)
        painter.fillRect(0, j * unit, width(), 1, white128);
    painter.fillRect(0, height() - 1, width(), 1, white128);
    for (int i = 0; i < w; ++i)
        painter.fillRect(i * unit, 0, 1, height(), white128);
    painter.fillRect(width() - 1, 0, 1, height(), white128);
}

void BrushEditorWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (rect().contains(event->pos()))
    {
        const auto &[x, y] = event->pos();
        const int unit = grid_size * zoom;

        emit cursorPositionChanged({x / unit, y / unit});

        update();
    }
}

void BrushEditorWidget::mousePressEvent(QMouseEvent *event)
{
    const auto &[x, y] = event->pos();
    const int unit = grid_size * zoom;

    if (event->button() == Qt::LeftButton)
        toggleAt({x / unit, y / unit});

    if (event->button() == Qt::RightButton)
        brush_pixels.setPixelColor(x / unit, y / unit, Qt::black);

    update();
}
