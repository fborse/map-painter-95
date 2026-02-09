#pragma once

#include <QWidget>

class ColorHueWidget: public QWidget
{
    Q_OBJECT
public:
    explicit ColorHueWidget(QWidget *parent = nullptr);

    int getHueAt(const int y) const { return y * 360 / height(); }
    int getHue() const { return getHueAt(hue); }

public slots:
    void setHueHeight(const int y) { hue = y; update(); }

signals:
    void hueChanged(const int hue);

private:
    int hue;

    void paintEvent(QPaintEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};
