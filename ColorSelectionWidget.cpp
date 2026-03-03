#include "ColorSelectionWidget.hpp"

#include <QPainter>
#include <QMouseEvent>

ColorSelectionWidget::ColorSelectionWidget(QWidget *parent):
    QWidget(parent),
    hue{0}, saturation{1}, value{1}, alpha{255}
{
    setFixedSize(16, 16);
}

void ColorSelectionWidget::changeColor(const double h, const double s, const double v, const int a)
{
    hue = h;
    saturation = s;
    value = v;
    alpha = a;

    emit colorChanged(getColor());
    update();
}

void ColorSelectionWidget::setHue(const double h)
{
    changeColor(h, saturation, value, alpha);
}

void ColorSelectionWidget::setSaturation(const double s)
{
    changeColor(hue, s, value, alpha);
}

void ColorSelectionWidget::setValue(const double v)
{
    changeColor(hue, saturation, v, alpha);
}

void ColorSelectionWidget::setAlpha(const int a)
{
    changeColor(hue, saturation, value, a);
}

void ColorSelectionWidget::setColor(const QColor color)
{
    changeColor(color.hsvHueF(), color.saturationF(), color.valueF(), color.alpha());
}

void ColorSelectionWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    const auto &[w, h] = size();
    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};

//  TODO: handle uneven sizes
    painter.fillRect(0, 0, w/2, h/2, dark);
    painter.fillRect(w/2, 0, w/2, h/2, light);
    painter.fillRect(0, h/2, w/2, h/2, light);
    painter.fillRect(w/2, h/2, w/2, h/2, dark);

    painter.fillRect(0, 0, w, h, getColor());
}

void ColorSelectionWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit clicked(getColor());
}
