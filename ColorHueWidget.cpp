#include "ColorHueWidget.hpp"

#include <QPainter>
#include <QMouseEvent>

ColorHueWidget::ColorHueWidget(QWidget *parent):
    QWidget(parent),
    hue{0}
{}

void ColorHueWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    for (int y = 0; y < height(); ++y)
        painter.fillRect(0, y, width(), 1, QColor::fromHsv(getHueAt(y), 255, 255));

    painter.fillRect(0, hue - 1, width(), 3, Qt::black);
    painter.fillRect(0, hue, width(), 1, Qt::white);
}

static inline int clamp(const int lower, const int y, const int upper)
{
    if (y < lower)
        return lower;
    else if (y > upper)
        return upper;
    else
        return y;
}

void ColorHueWidget::mouseMoveEvent(QMouseEvent *event)
{
    hue = clamp(0, event->pos().y(), height() - 1);
    emit hueChanged(getHue());
    update();
}

void ColorHueWidget::mousePressEvent(QMouseEvent *event)
{
    hue = event->pos().y();
    emit hueChanged(getHue());
    update();
}
