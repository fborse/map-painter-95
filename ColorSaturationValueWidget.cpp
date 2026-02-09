#include "ColorSaturationValueWidget.hpp"

#include <QPainter>
#include <QMouseEvent>

//  depends on the initial widget size
//  DO NOT RELY ON IT !
static const int magic_value = 198;

ColorSaturationValueWidget::ColorSaturationValueWidget(QWidget *parent):
    QWidget(parent),
    hue{0}, saturation{magic_value}, value{0}
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

    for (int y = 0; y < height(); ++y)
        for (int x = 0; x < width(); ++x)
            painter.fillRect(x, y, 1, 1, QColor::fromHsv(hue, getSaturationAt(x), getValueAt(y)));

    painter.setPen(pen(Qt::black, 4));
    painter.drawPoint(saturation, value);
    painter.setPen(pen(Qt::white, 2));
    painter.drawPoint(saturation, value);
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
    saturation = clamp(0, x, width());
    value = clamp(0, y, height());

    emit saturationChanged(getSaturation());
    emit valueChanged(getValue());

    update();
}

void ColorSaturationValueWidget::mousePressEvent(QMouseEvent *event)
{
    const auto &[x, y] = event->pos();
    saturation = x;
    value = y;

    emit saturationChanged(getSaturation());
    emit valueChanged(getValue());

    update();
}
