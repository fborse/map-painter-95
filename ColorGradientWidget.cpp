#include "ColorGradientWidget.hpp"

#include <QPainter>
#include <QMouseEvent>

ColorGradientWidget::ColorGradientWidget(QWidget *parent):
    QWidget(parent),
    left{Qt::black}, right{Qt::white},
    clicked{false}
{
}

static inline int lin(const int a, const int b, const int x, const int dx)
{
    return a + (b - a) * (x / double(dx));
}

QColor ColorGradientWidget::getColorAt(const int x) const
{
    const int w = width();

    return {
        lin(left.red(), right.red(), x, w),
        lin(left.green(), right.green(), x, w),
        lin(left.blue(), right.blue(), x, w),
        lin(left.alpha(), right.alpha(), x, w)
    };
}

void ColorGradientWidget::paintBackground(QPainter &painter) const
{
    const auto &[w, h] = size();
    const int s = qCeil(h / 2.0);

    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};

//  overshoots are cheap
    for (int i = 0; i < w / s + 1; ++i)
        painter.fillRect(i * s, 0, s, s, (i % 2)? dark : light);
    for (int i = 0; i < w / s + 1; ++i)
        painter.fillRect(i * s, s, s, s, (i % 2)? light : dark);
}

void ColorGradientWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    paintBackground(painter);

    for (int x = 0; x < width(); ++x)
        painter.fillRect(x, 0, 1, height(), getColorAt(x));
}

static inline int clamp(const int lower, const int x, const int upper)
{
    if (x < lower)
        return lower;
    else if (x > upper)
        return upper;
    else
        return x;
}

void ColorGradientWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (clicked)
    {
        const int x = clamp(0, event->pos().x(), width());
        emit colorSelected(getColorAt(x));
    }
}

void ColorGradientWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        clicked = true;
        emit colorSelected(getColorAt(event->pos().x()));
    }
}

void ColorGradientWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        clicked = false;
}
