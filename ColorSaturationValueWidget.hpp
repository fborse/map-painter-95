#pragma once

#include <QWidget>

class ColorSaturationValueWidget: public QWidget
{
    Q_OBJECT
public:
    explicit ColorSaturationValueWidget(QWidget *parent = nullptr);

public slots:
    void setHue(const double h) { hue = h; update(); }
    void setSaturation(const double s) { saturation = s; update(); }
    void setValue(const double v) { value = v; update(); }

signals:
    void saturationChanged(const double saturation);
    void valueChanged(const double value);

private:
    double hue, saturation, value;

//  only used for painting the widget
    double getSaturationAt(const int x) const { return x / double(width()); }
    double getValueAt(const int y) const { return (height()-y) / double(height()); }

    void paintEvent(QPaintEvent *) override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};
