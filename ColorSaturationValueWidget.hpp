#pragma once

#include <QWidget>

class ColorSaturationValueWidget: public QWidget
{
    Q_OBJECT
public:
    explicit ColorSaturationValueWidget(QWidget *parent = nullptr);

    int getSaturationAt(const int x) const { return x * 255 / width(); }
    int getSaturation() const { return getSaturationAt(saturation); }

    int getValueAt(const int y) const { return (height()-y) * 255 / height(); }
    int getValue() const { return getValueAt(value); }

public slots:
    void setHue(const double h) { hue = h; update(); }
    void setSaturation(const int x) { saturation = x; update(); }
    void setValue(const int y) { value = y; update(); }

signals:
    void saturationChanged(const int saturation);
    void valueChanged(const int value);

private:
    double hue;
    int saturation, value;

    void paintEvent(QPaintEvent *) override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};
