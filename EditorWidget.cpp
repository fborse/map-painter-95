#include "EditorWidget.hpp"

#include <QPainter>

EditorWidget::EditorWidget(QWidget *parent):
    QWidget(parent),
    grid_aspect{20, 16}, tilesize{32}, zoom{1},
    undo_stack{nullptr}
{
}

void EditorWidget::resize()
{
    setFixedSize(grid_aspect * tilesize * zoom);

    update();
}

//  QPoint's division rounds ; we DON'T want that
static inline QPoint divide(const QPoint &p, const int a)
{
    return {int(p.x() / a), int(p.y() / a)};
}

QRect EditorWidget::asLocalRect(const QPoint &p1, const QPoint &p2) const
{
    const auto &[x1, y1] = divide(p1, tilesize);
    const auto &[x2, y2] = divide(p2, tilesize);

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
