#pragma once

#include <QWidget>

class BrushEditorWidget: public QWidget
{
    Q_OBJECT
public:
    explicit BrushEditorWidget(QWidget *parent = nullptr);

    QImage getBrushPixels() const { return brush_pixels; }
    void setBrushPixels(const QImage &pixels) { brush_pixels = pixels; resize(); }

    void resize();

public slots:
    void setZoom(const double z) { zoom = z; resize(); }
    void setWidth(const int w) { resizePixels({w, brush_pixels.height()}); }
    void setHeight(const int h) { resizePixels({brush_pixels.width(), h}); }

signals:
    void cursorPositionChanged(const QPoint position);

private:
    static const int grid_size = 2;

    double zoom;
    QImage brush_pixels;

    void toggleAt(const QPoint &p);
    void resizePixels(const QSize &size);

    void paintEvent(QPaintEvent *) override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};
