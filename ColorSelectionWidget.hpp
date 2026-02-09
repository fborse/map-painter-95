#pragma once

#include <QWidget>

class ColorSelectionWidget: public QWidget
{
    Q_OBJECT
public:
    explicit ColorSelectionWidget(QWidget *parent = nullptr);

    QColor getColor() const { return QColor::fromHsv(hue, saturation, value, alpha); }

public slots:
    void setHue(const int h);
    void setSaturation(const int s);
    void setValue(const int v);
    void setAlpha(const int a);

    void setColor(const QColor color);

signals:
    void clicked(const QColor color);
    void colorChanged(const QColor color);

private:
    int hue, saturation, value, alpha;

//  used by setHue, setSaturation, setValue and setAlpha
    void changeColor(const int h, const int s, const int v, const int a);

    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
};
