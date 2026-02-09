#include "MapPainterWidget.hpp"

#include <QPainter>
#include <QMouseEvent>

MapPainterWidget::MapPainterWidget(QWidget *parent):
    EditorWidget(parent),
    show_grid{false}, draw_tool{PEN},
    draw_color{Qt::red}, pen_size{1},
    mouse_cursor{}, click_origin{}, right_click{false}
{}

void MapPainterWidget::setDrawTool(const int index)
{
    Q_ASSERT(0 <= index && index < 9);
    draw_tool = DrawTool(index);
}

void MapPainterWidget::paintCursor(QPainter &/*painter*/) const
{}

void MapPainterWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    paintBackground(painter);

    painter.scale(zoom, zoom);
    paintLayers(painter);
    painter.resetTransform();

    if (show_grid)
        paintGrid(painter);
    paintCursor(painter);
}

//  QPoint's division rounds ; we DON'T want that
static inline QPoint divide(const QPoint &p, const int a)
{
    return {int(p.x() / a), int(p.y() / a)};
}

QColor MapPainterWidget::getColorAt(const QPoint &p) const
{
    if (!(map_layers && tileset))
        return Qt::transparent;

    const auto &[i, j] = divide(p, tilesize);
    const auto &[x, y] = p - QPoint(i, j) * tilesize;

    if (j < 0 || j >= map_layers->length() || i < 0 || i >= map_layers->at(j).length())
        return Qt::transparent;

    const auto id = map_layers->at(j).at(i);
    if (id.isEmpty())
        return Qt::transparent;

    if (tileset->contains(id))
        return tileset->value(id).pixelColor(x, y);
    else
        return Qt::transparent;
}

void MapPainterWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouse_cursor = divide(event->pos(), zoom);

    if (click_origin)
        switch (draw_tool)
        {
        case PEN:
        case BRUSH:
        case ERASER:
        case SHADER:
            drag_points.push_back(mouse_cursor); break;
        case LINE:
        case SHAPE:
            drag_points = {mouse_cursor}; break;
        case PIPETTE:
            emit colorChanged(getColorAt(mouse_cursor));
        default:
            break;
        }

    if (right_click)
        emit colorChanged(getColorAt(mouse_cursor));
}

void MapPainterWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        click_origin = divide(event->pos(), zoom);

        drag_points = {mouse_cursor};
        if (draw_tool == PIPETTE)
            emit colorChanged(getColorAt(mouse_cursor));
    }

    if (event->button() == Qt::RightButton)
    {
        right_click = true;
        emit colorChanged(getColorAt(mouse_cursor));
    }
}

void MapPainterWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        click_origin = {};
        drag_points = {};
    }

    if (event->button() == Qt::RightButton)
        right_click = false;
}
