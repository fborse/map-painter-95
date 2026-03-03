#pragma once

#include <QWidget>

class ColorHueWidget: public QWidget
{
    Q_OBJECT
public:
    explicit ColorHueWidget(QWidget *parent = nullptr);

public slots:
    void setHue(const double h) { hue = h; update(); }

signals:
    void hueChanged(const double hue);

private:
    double hue;

//  only used for painting the widget
    int getHueAt(const int y) const { return y * 360 / height(); }

    void paintEvent(QPaintEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};
