#pragma once

#include <QWidget>

class ColorGradientWidget: public QWidget
{
    Q_OBJECT
public:
    explicit ColorGradientWidget(QWidget *parent = nullptr);

public slots:
    void setLeftColor(const QColor color) { left = color; update(); }
    void setRightColor(const QColor color) { right = color; update(); }

    QColor getColorAt(const int x) const;

signals:
    void colorSelected(const QColor color);

private:
    QColor left, right;
    bool clicked;

    void paintBackground(QPainter &painter) const;
    void paintEvent(QPaintEvent *) override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};
