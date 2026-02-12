#include "MapPainterWidget.hpp"

#include <QPainter>
#include <QMouseEvent>
#include <QUuid>

template <typename T>
static inline QSharedPointer<T> lock_ptr(QWeakPointer<T> &weak)
{
    auto shared = weak.toStrongRef();
    Q_ASSERT(!shared.isNull());

    return shared;
}

class TilesOrderCommand
{
public:
    TilesOrderCommand(QWeakPointer<Names> tiles_order): tiles_order_ptr{tiles_order} {}
    QSharedPointer<Names> lockTilesOrder() { return lock_ptr(tiles_order_ptr); }

private:
    QWeakPointer<Names> tiles_order_ptr;
};

class TilesetCommand
{
public:
    TilesetCommand(QWeakPointer<Tileset> tileset): tileset_ptr{tileset} {}
    QSharedPointer<Tileset> lockTileset() { return lock_ptr(tileset_ptr); }

private:
    QWeakPointer<Tileset> tileset_ptr;
};

class MapLayersCommand
{
public:
    MapLayersCommand(QWeakPointer<MapLayer> map_layers): map_layers_ptr{map_layers} {}
    QSharedPointer<MapLayer> lockMapLayers() { return lock_ptr(map_layers_ptr); }

private:
    QWeakPointer<MapLayer> map_layers_ptr;
};

class ReplaceTilesCommand final: public QUndoCommand, public TilesetCommand
{
public:
    using Changes = QHash<TileReference, QImage>;

    ReplaceTilesCommand(QWeakPointer<Tileset> tileset, const Changes &prev, const Changes &next):
        QUndoCommand(), TilesetCommand(tileset), prev{prev}, next{next}
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

class AddTilesCommand final: public QUndoCommand, public TilesOrderCommand, public TilesetCommand
{
public:
    using Added = QMap<TileReference, QImage>;

    AddTilesCommand(QWeakPointer<Names> tiles_order, QWeakPointer<Tileset> tileset, const Added &added):
        QUndoCommand(), TilesOrderCommand(tiles_order), TilesetCommand(tileset), added{added}
    {}

    void undo() final override
    {
    //  they got added at the end, contiguously, and removal order does not matter
        for (auto &id: added.keys())
        {
            lockTilesOrder()->pop_back();
            lockTileset()->remove(id);
        }
    }

    void redo() final override
    {
    //  added is likely small, so retrieval should be short
        for (auto &id: added.keys())
        {
            lockTilesOrder()->push_back(id);
            lockTileset()->insert(id, added.value(id));
        }
    }

private:
    Added added;
};

class ReplaceReferencesCommand final: public QUndoCommand, public MapLayersCommand
{
public:
    using Changes = QHash<QPoint, TileReference>;

    ReplaceReferencesCommand(QWeakPointer<MapLayer> map_layers, const Changes &prev, const Changes &next):
        QUndoCommand(), MapLayersCommand(map_layers), prev{prev}, next{next}
    {}

    void undo() final override
    {
        for (auto &p: prev.keys())
            (*lockMapLayers())[p.y()][p.x()] = prev[p];
    }

    void redo() final override
    {
        for (auto &p: next.keys())
            (*lockMapLayers())[p.y()][p.x()] = next[p];
    }

private:
    Changes prev, next;
};

MapPainterWidget::MapPainterWidget(QWidget *parent):
    EditorWidget(parent),
    show_grid{false}, draw_tool{PEN}, retroactive{true},
    draw_color{Qt::red},
    pen_size{1}, anti_aliasing{false}, round_pen_corners{false},
    ellipse_shape{false}, fill_shape{false}, rect_radius{false},
    fill_tolerance{0}, fill_this_tile_only{true},
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

//  QPoint's division rounds ; we DON'T want that
static inline QPoint divide(const QPoint &p, const int a)
{
    return {int(p.x() / a), int(p.y() / a)};
}

static inline bool similar(const QColor &c1, const QColor &c2, const double tolerance)
{
    const double dr = c1.redF() - c2.redF();
    const double dg = c1.greenF() - c2.greenF();
    const double db = c1.blueF() - c2.blueF();
    const double da = c1.alphaF() - c2.alphaF();

    const double dist = dr*dr + dg*dg + db*db + da*da;

    return (dist <= tolerance/100*tolerance/100);
}

void MapPainterWidget::drawFill(QImage &original) const
{
    const QRect rect = fill_this_tile_only?
        QRect(divide(mouse_cursor, tilesize) * tilesize, QSize(tilesize, tilesize))
      : getWidgetRect();

    const QColor original_color = original.pixelColor(mouse_cursor);
    const QPoint d[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    QSet<QPoint> done;
//  TODO: change QVector to a better structure
    QVector<QPoint> todo = {mouse_cursor};
    while (!todo.isEmpty())
    {
        const QPoint p = todo.takeAt(0);
        original.setPixelColor(p, draw_color);
        done.insert(p);

        for (auto &dp: d)
            if (rect.contains(p + dp))
                if (!done.contains(p + dp) && !todo.contains(p + dp))
                    if (similar(original.pixelColor(p + dp), original_color, fill_tolerance))
                        todo.push_back(p + dp);
    }
}

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
            drawFill(original);
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

static inline auto get_prev_next_images(const QHash<QPoint, QHash<QPoint, QColor>> &changes, const Tileset &tileset, const MapLayer &map_layers, std::function<bool(TileReference)> fn)
{
    ReplaceTilesCommand::Changes prev, next;

    for (auto &q: changes.keys())
    {
        for (auto &p: changes[q].keys())
        {
            const auto id = map_layers.at(q.y()).at(q.x());

            if (fn(id))
            {
                if (!prev.contains(id))
                {
                    prev.insert(id, tileset.value(id));
                    next.insert(id, tileset.value(id));
                }

                next[id].setPixelColor(p, changes[q][p]);
            }
        }
    }

    return QPair<decltype(prev), decltype(next)>(prev, next);
}

void MapPainterWidget::handleRetroactiveDrawing(const QHash<QPoint, QHash<QPoint, QColor>> &changed_pixels) const
{
    const auto &[prev, next] = get_prev_next_images(
        changed_pixels,
        *tileset,
        *map_layers,
        [&] (TileReference id) { return !id.isEmpty(); }
    );

    undo_stack->push(new ReplaceTilesCommand(tileset, prev, next));
}

static inline bool is_tile_unique(const MapLayer &map_layers, const TileReference &id)
{
    int found = 0;

    for (auto &row: map_layers)
        for (auto &tile: row)
            if (tile == id)
                if (++found > 1)
                    return false;

    return true;
}

void MapPainterWidget::handleNonRetroactiveDrawing(const QHash<QPoint, QHash<QPoint, QColor>> &changed_pixels) const
{
    const auto &[prev_tiles, next_tiles] = get_prev_next_images(
        changed_pixels,
        *tileset,
        *map_layers,
        [&] (TileReference id) { return is_tile_unique(*map_layers, id) && !id.isEmpty(); }
    );

    AddTilesCommand::Added added;
    ReplaceReferencesCommand::Changes prev_refs, next_refs;
    for (auto &q: changed_pixels.keys())
    {
        const TileReference prev_id = map_layers->at(q.y()).at(q.x());

    //  faster than !is_tile_unique(*map_layers, prev_id)
        if (prev_id.isEmpty() || !prev_tiles.contains(prev_id))
        {
            QImage new_image = tileset->value(prev_id);
            for (auto &p: changed_pixels[q].keys())
                new_image.setPixelColor(p, changed_pixels[q][p]);

            const QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
            added[uuid] = new_image;
            prev_refs[q] = prev_id;
            next_refs[q] = uuid;
        }
    }

    if (!next_tiles.isEmpty() || !added.isEmpty() || !next_refs.isEmpty())
    {
        undo_stack->beginMacro("non-retroactive change");
        undo_stack->push(new ReplaceTilesCommand(tileset, prev_tiles, next_tiles));
        undo_stack->push(new AddTilesCommand(tiles_order, tileset, added));
        undo_stack->push(new ReplaceReferencesCommand(map_layers, prev_refs, next_refs));
        undo_stack->endMacro();
    }
}

static inline auto get_changed_pixels(const QImage &original, const QImage &changed, const int tilesize)
{
    QHash<QPoint, QHash<QPoint, QColor>> changes;

//  please be fast (:
    for (int j = 0; j < original.height(); ++j)
    {
        for (int i = 0; i < original.width(); ++i)
        {
            const QPoint p = {i, j};
            const QPoint q = divide(p, tilesize);

            if (original.pixelColor(p) != changed.pixelColor(p))
                changes[q][p - q * tilesize] = changed.pixelColor(p);
        }
    }

    return changes;
}

void MapPainterWidget::handleDrawChanges() const
{
    if (!(tileset && map_layers))
        return;

    const auto changed_pixels = get_changed_pixels(getPaintedLayer(), getDrawnLayer(), tilesize);
    if (changed_pixels.isEmpty())
        return;

    if (retroactive)
        handleRetroactiveDrawing(changed_pixels);
    else
        handleNonRetroactiveDrawing(changed_pixels);
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
