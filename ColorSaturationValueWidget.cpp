#include "ColorSaturationValueWidget.hpp"

#include <QPainter>
#include <QMouseEvent>

ColorSaturationValueWidget::ColorSaturationValueWidget(QWidget *parent):
    QWidget(parent),
    hue{0}, saturation{1}, value{1}
{
}

static inline QPen pen(const QColor &color, const int width)
{
    QPen p(color);
    p.setWidth(width);

    return p;
}

void ColorSaturationValueWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    const auto &[w, h] = size();

    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            painter.fillRect(x, y, 1, 1, QColor::fromHsvF(hue, getSaturationAt(x), getValueAt(y)));

    painter.setPen(pen(Qt::black, 4));
    painter.drawPoint(saturation * w, (1-value) * h);
    painter.setPen(pen(Qt::white, 2));
    painter.drawPoint(saturation * w, (1-value) * h);
}

static inline int clamp(const int lower, const int val, const int upper)
{
    if (val < lower)
        return lower;
    else if (val > upper)
        return upper;
    else
        return val;
}

void ColorSaturationValueWidget::mouseMoveEvent(QMouseEvent *event)
{
    const auto &[x, y] = event->pos();
    const auto &[w, h] = size();

    saturation = clamp(0, x, w) / double(w);
    value = clamp(0, h-y, h) / double(h);

    emit saturationChanged(saturation);
    emit valueChanged(value);

    update();
}

void ColorSaturationValueWidget::mousePressEvent(QMouseEvent *event)
{
    const auto &[x, y] = event->pos();
    const auto &[w, h] = size();

    saturation = x / double(w);
    value = (h-y) / double(h);

    emit saturationChanged(saturation);
    emit valueChanged(value);

    update();
}
