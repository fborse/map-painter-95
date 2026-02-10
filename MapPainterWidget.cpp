#include "MapPainterWidget.hpp"

#include <QPainter>
#include <QMouseEvent>

class TilesetCommand
{
public:
    TilesetCommand(QWeakPointer<Names> tiles_order, QWeakPointer<Tileset> tileset):
        tiles_order_ptr{tiles_order}, tileset_ptr{tileset}
    {}

    QSharedPointer<Names> lockTilesOrder()
    {
        auto ptr = tiles_order_ptr.toStrongRef();
        Q_ASSERT(!ptr.isNull());

        return ptr;
    }

    QSharedPointer<Tileset> lockTileset()
    {
        auto ptr = tileset_ptr.toStrongRef();
        Q_ASSERT(!ptr.isNull());

        return ptr;
    }

private:
    QWeakPointer<Names> tiles_order_ptr;
    QWeakPointer<Tileset> tileset_ptr;
};

class ReplaceTilesCommand final: public QUndoCommand, public TilesetCommand
{
public:
    using Changes = QHash<TileReference, QImage>;

    ReplaceTilesCommand(QWeakPointer<Tileset> tileset, const Changes &prev, const Changes &next):
        QUndoCommand(), TilesetCommand({}, tileset), prev{prev}, next{next}
    {}

    void undo() final override
    {
        for (auto &id: prev.keys())
            (*lockTileset())[id] = prev[id];
    }

    void redo() final override
    {
        for (auto &id: next.keys())
            (*lockTileset())[id] = next[id];
    }

private:
    Changes prev, next;
};

MapPainterWidget::MapPainterWidget(QWidget *parent):
    EditorWidget(parent),
    show_grid{false}, draw_tool{PEN},
    draw_color{Qt::red},
    pen_size{1}, anti_aliasing{false}, round_pen_corners{false},
    ellipse_shape{false}, fill_shape{false}, rect_radius{false},
    mouse_cursor{}, click_origin{}, right_click{false}
{}

void MapPainterWidget::setDrawTool(const int index)
{
    Q_ASSERT(0 <= index && index < 9);
    draw_tool = DrawTool(index);
}

void MapPainterWidget::paintCursor(QPainter &/*painter*/) const
{}

QColor MapPainterWidget::getEffectiveDrawColor() const
{
    const QColor dark32 = {0, 0, 0, 32};
    const QColor light32 = {255, 255, 255, 32};

    if (draw_tool != SHADER)
        return draw_color;
    else if (true)
        return dark32;
    else
        return light32;
}

QPen MapPainterWidget::getPen() const
{
    QPen pen(getEffectiveDrawColor());
    pen.setWidth(pen_size);
    pen.setCapStyle(round_pen_corners? Qt::RoundCap : Qt::SquareCap);
    pen.setJoinStyle(round_pen_corners? Qt::RoundJoin : Qt::BevelJoin);

    return pen;
}

void MapPainterWidget::setPen(QPainter &painter) const
{
    painter.setPen(getPen());
    painter.setRenderHint(QPainter::Antialiasing, anti_aliasing);
}

void MapPainterWidget::drawPen(QPainter &painter) const
{
    setPen(painter);
    painter.drawPolyline(drag_points.data(), drag_points.length());
}

void MapPainterWidget::drawLine(QPainter &painter) const
{
    setPen(painter);
    painter.drawLine(*click_origin, mouse_cursor);
}

void MapPainterWidget::drawBrush(QPainter &painter) const
{
    setPen(painter);
//  TODO: create brush controls
}

static inline QRect to_rect(const QPoint &p1, const QPoint &p2)
{
    return {
        qMin(p1.x(), p2.x()), qMin(p1.y(), p2.y()),
        qAbs(p1.x() - p2.x()), qAbs(p1.y() - p2.y())
    };
}

void MapPainterWidget::drawShape(QPainter &painter) const
{
    setPen(painter);
    if (fill_shape)
        painter.setBrush(draw_color);

    const QRect r = to_rect(*click_origin, mouse_cursor);
    if (ellipse_shape)
        painter.drawEllipse(r);
    else
        painter.drawRoundedRect(r, rect_radius, rect_radius);
}

void MapPainterWidget::drawFill(QImage &/*original*/) const
{}

void MapPainterWidget::drawEraser(QPainter &painter) const
{
    setPen(painter);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.drawPolyline(drag_points.data(), drag_points.length());
}

void MapPainterWidget::drawShader(QPainter &painter) const
{
    setPen(painter);
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    painter.drawPolyline(drag_points.data(), drag_points.length());
}

QImage MapPainterWidget::getDrawnLayer() const
{
    QImage original = getPaintedLayer();
    if (click_origin)
    {
        QPainter painter(&original);

        if (draw_tool == PEN)
            drawPen(painter);
        else if (draw_tool == LINE)
            drawLine(painter);
        else if (draw_tool == BRUSH)
            drawBrush(painter);
        else if (draw_tool == SHAPE)
            drawShape(painter);
        else if (draw_tool == FILL)
        {}
        else if (draw_tool == ERASER)
            drawEraser(painter);
        else if (draw_tool == SHADER)
            drawShader(painter);
    }

    return original;
}

void MapPainterWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    paintBackground(painter);

    painter.scale(zoom, zoom);
    painter.drawImage(0, 0, getDrawnLayer());
    painter.resetTransform();

    if (show_grid)
        paintGrid(painter);
    paintCursor(painter);
}

QColor MapPainterWidget::getColorAt(const QPoint &p) const
{
    if (!(map_layers && tileset))
        return Qt::transparent;
    else if (p.x() < 0 || p.y() < 0)
        return Qt::transparent;

    const QImage layer = getPaintedLayer();
    if (p.x() < layer.width() && p.y() < layer.height())
        return layer.pixelColor(p);
    else
        return Qt::transparent;
}

//  QPoint's division rounds ; we DON'T want that
static inline QPoint divide(const QPoint &p, const int a)
{
    return {int(p.x() / a), int(p.y() / a)};
}

static inline auto get_changes(const QImage &original, const QImage &changed)
{
    QHash<QPoint, QColor> changes;

//  please be fast (:
    for (int j = 0; j < original.height(); ++j)
        for (int i = 0; i < original.width(); ++i)
            if (original.pixelColor(i, j) != changed.pixelColor(i, j))
                changes[{i, j}] = changed.pixelColor(i, j);

    return changes;
}

void MapPainterWidget::handleRetroactiveDrawing(const QHash<QPoint, QColor> &changed_pixels) const
{
    ReplaceTilesCommand::Changes prev, next;
    for (auto &p: changed_pixels.keys())
    {
        const auto q = divide(p, tilesize);
        const auto id = map_layers->at(q.y()).at(q.x());

        if (!id.isEmpty())
        {
            if (!prev.contains(id))
            {
                prev.insert(id, tileset->value(id));
                next.insert(id, tileset->value(id));
            }

            next[id].setPixelColor(p - q * tilesize, changed_pixels[p]);
        }
    }

    undo_stack->push(new ReplaceTilesCommand(tileset, prev, next));
}

void MapPainterWidget::handleDrawChanges() const
{
    if (!(tileset && map_layers))
        return;

    const auto changed_pixels = get_changes(getPaintedLayer(), getDrawnLayer());
    if (changed_pixels.isEmpty())
        return;

    if (true)
        handleRetroactiveDrawing(changed_pixels);
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
        case PIPETTE:
            emit colorChanged(getColorAt(mouse_cursor));
        default:
            break;
        }

    if (right_click)
        emit colorChanged(getColorAt(mouse_cursor));

    update();
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

    update();
}

void MapPainterWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        handleDrawChanges();

        click_origin = {};
        drag_points = {};
    }

    if (event->button() == Qt::RightButton)
        right_click = false;

    update();
}
