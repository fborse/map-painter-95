#pragma once

#include <QWidget>

class BrushDisplayWidget: public QWidget
{
    Q_OBJECT
public:
    explicit BrushDisplayWidget(QWidget *parent = nullptr);

public slots:
    void setBrushPixels(const QImage &pixels) { brush_pixels = pixels; resize(); }

signals:
    void brushChanged(const QImage pixels);

private:
    static const int grid_size = 8;

    QImage brush_pixels;

    void resize();

    void paintEvent(QPaintEvent *) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};
