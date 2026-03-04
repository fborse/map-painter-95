#include "EditorWidget.hpp"

#include <QPainter>
#include <QWheelEvent>

EditorWidget::EditorWidget(QWidget *parent):
    QWidget(parent),
    grid_aspect{20, 16}, tilesize{32}, zoom{1},
    current_layer{0}, current_frame{0},
    undo_stack{nullptr}, tiles_order{nullptr}, simple_tiles{nullptr},
    selected_tiles{nullptr}, map_layers{nullptr}
{
}

void EditorWidget::setCurrentLayer(const int layer)
{
    if (layer < 0)
        current_layer = 0;
    else
        current_layer = layer;

    update();
}

void EditorWidget::setCurrentFrame(const int frame)
{
    if (frame < 0)
        current_frame = 0;
    else
        current_frame = frame;

    update();
}

void EditorWidget::resize()
{
    setFixedSize(grid_aspect * tilesize * zoom);

    update();
}

//  QPoint's division rounds ; we DON'T want that
static inline QPoint divide(const QPoint &p, const double a)
{
    return {int(p.x() / a), int(p.y() / a)};
}

QRect EditorWidget::asLocalRect(const QPoint &p1, const QPoint &p2) const
{
    const double unit = tilesize * zoom;
    const auto &[x1, y1] = divide(p1, unit);
    const auto &[x2, y2] = divide(p2, unit);

    return QRect(
        QPoint(qMin(x1, x2), qMin(y1, y2)),
        QPoint(qMax(x1, x2), qMax(y1, y2))
    );
}

void EditorWidget::paintBackground(QPainter &painter) const
{
    const double unit = tilesize * zoom;
    const int s2 = qCeil(unit / 2);

    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};

    for (int j = 0; j < grid_aspect.height(); ++j)
    {
        for (int i = 0; i < grid_aspect.width(); ++i)
        {
            painter.fillRect(i * unit, j * unit, s2, s2, dark);
            painter.fillRect(i * unit + s2, j * unit, s2, s2, light);
            painter.fillRect(i * unit, j * unit + s2, s2, s2, light);
            painter.fillRect(i * unit + s2, j * unit + s2, s2, s2, dark);
        }
    }
}

QImage EditorWidget::getPaintedLayer(const int layer) const
{
    Q_ASSERT(!map_layers.isNull());
    Q_ASSERT(!simple_tiles.isNull());

    QImage img(grid_aspect * tilesize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    const auto &_layer = map_layers->at(layer);

    QPainter painter(&img);
    for (int j = 0; j < _layer.length(); ++j)
    {
        for (int i = 0; i < _layer.at(j).length(); ++i)
        {
            const auto id = _layer.at(j).at(i);

            if (simple_tiles->contains(id))
            {
                const auto &frames = (*simple_tiles)[id].frames;
                const int n = frames.length();
                painter.drawImage(i * tilesize, j * tilesize, frames[qMin(current_frame, n-1)]);
            }
        }
    }

    return img;
}

void EditorWidget::paintGrid(QPainter &painter) const
{
    const double unit = tilesize * zoom;
    const auto &[w, h] = grid_aspect * unit;

    const QColor white128 = {255, 255, 255, 128};

    for (int j = 0; j < grid_aspect.height(); ++j)
        painter.fillRect(0, j * unit, w, 1, white128);
    for (int i = 0; i < grid_aspect.width(); ++i)
        painter.fillRect(i * unit, 0, 1, h, white128);
    for (int j = 1; j <= grid_aspect.height(); ++j)
        painter.fillRect(0, j * unit - 1, w, 1, white128);
    for (int i = 1; i <= grid_aspect.width(); ++i)
        painter.fillRect(i * unit - 1, 0, 1, h, white128);
}

void EditorWidget::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier)
    {
        const double dy = event->angleDelta().y() / 30.0;
        emit zoomSet(zoom + dy / 10);
        emit scrolledAt(event->position().toPoint());
    }
    else
    {
        event->ignore();
    }
}
