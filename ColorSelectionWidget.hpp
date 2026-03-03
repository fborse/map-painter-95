#pragma once

#include <QWidget>

class ColorSelectionWidget: public QWidget
{
    Q_OBJECT
public:
    explicit ColorSelectionWidget(QWidget *parent = nullptr);

    QColor getColor() const { return QColor::fromHsvF(hue, saturation, value, alpha / 255.0); }

public slots:
    void setHue(const double h);
    void setSaturation(const double s);
    void setValue(const double v);
    void setAlpha(const int a);

    void setColor(const QColor color);

signals:
    void clicked(const QColor color);
    void colorChanged(const QColor color);

private:
    double hue, saturation, value;
    int alpha;

//  used by setHue, setSaturation, setValue and setAlpha
    void changeColor(const double h, const double s, const double v, const int a);

    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
};
