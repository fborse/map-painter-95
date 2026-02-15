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

//  QPoint's division rounds ; we DON'T want that
static inline QPoint divide(const QPoint &p, const int a)
{
    return {int(p.x() / a), int(p.y() / a)};
}

MapPainterWidget::MapPainterWidget(QWidget *parent):
    EditorWidget(parent),
    show_grid{false}, draw_tool{PEN}, retroactive{true},
    draw_color{Qt::red},
    pen_size{1}, anti_aliasing{false}, round_pen_corners{false},
    brush_pixels{},
    ellipse_shape{false}, fill_shape{false}, rect_radius{false},
    fill_tolerance{0}, fill_this_tile_only{true},
    darken{true},
    selection_shape{RECTANGLE}, selection_color_key{false},
    mouse_cursor{}, click_origin{}, right_click{false},
    selection_rect{}, original_rect{}, selection_image{}, move_offset{}, magic_points{},
    cursor_image{}
{
    redrawCursorImage();
}

void MapPainterWidget::resize()
{
    if (map_layers)
    {
        const int h = map_layers->length();
        const int w = map_layers->at(0).length();

        grid_aspect = {w, h};
        EditorWidget::resize();
    }
}


void MapPainterWidget::setDrawTool(const int index)
{
    if (draw_tool == SELECTION)
    {
        if (selection_rect && !selection_rect->isEmpty())
            blitSelection();

        selection_rect = {};
        original_rect = {};
        magic_points = {};
        emit canCopy(false);
        update();
    }

    Q_ASSERT(0 <= index && index < 9);
    draw_tool = DrawTool(index);
    redrawCursorImage();
}

void MapPainterWidget::setSelectionShape(const int index)
{
    if (index != int(selection_shape))
    {
        if (selection_rect && !selection_rect->isEmpty())
            blitSelection();

        selection_rect = {};
        original_rect = {};
        magic_points = {};
        emit canCopy(false);
    }

    Q_ASSERT(0 <= index && index < 3);
    selection_shape = SelectionShape(index);

    update();
}

static inline QImage get_low_res(const int pen_size, const bool round)
{
    QImage img(pen_size, pen_size, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::black);

    QPen pen(Qt::white);
    pen.setWidth(pen_size);
    if (round)
        pen.setCapStyle(Qt::RoundCap);

    QPainter painter(&img);
    painter.setPen(pen);
    painter.drawPoint(pen_size / 2, pen_size / 2);

    return img;
}

static inline void reduce_to_outline(QImage &img)
{
    QPoint d[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    for (int y = 0; y < img.height(); ++y)
    {
        for (int x = 0; x < img.width(); ++x)
        {
            bool diff = false;
            for (auto &[i, j]: d)
                if (img.rect().contains(x+i, y+j))
                    if (img.pixelColor(x, y) != img.pixelColor(x+i, y+j))
                        if (img.pixelColor(x+i, y+j) != Qt::transparent)
                            diff = true;
            if (!diff)
                img.setPixelColor(x, y, Qt::transparent);
        }
    }
}

void MapPainterWidget::redrawCursorImage()
{
    if (draw_tool == SHAPE || draw_tool == FILL || draw_tool == PIPETTE || draw_tool == SELECTION)
    {
        cursor_image = QImage(zoom, zoom, QImage::Format_ARGB32_Premultiplied);
        cursor_image.fill((draw_tool == FILL)? draw_color : Qt::transparent);

        QPainter painter(&cursor_image);
        painter.fillRect(0, 0, zoom, 1, Qt::black);
        painter.fillRect(0, 0, 1, zoom, Qt::black);
        painter.fillRect(0, zoom-1, zoom, 1, Qt::black);
        painter.fillRect(zoom-1, 0, 1, zoom, Qt::black);

        painter.fillRect(1, 1, zoom-2, 1, Qt::white);
        painter.fillRect(1, 1, 1, zoom-2, Qt::white);
        painter.fillRect(1, zoom-2, zoom-2, 1, Qt::white);
        painter.fillRect(zoom-2, 1, 1, zoom-2, Qt::white);
    }
    else
    {
        const int w = (draw_tool == BRUSH)?
            brush_pixels.width() * zoom
          : pen_size * zoom;
        const int h = (draw_tool == BRUSH)?
            brush_pixels.height() * zoom
          : pen_size * zoom;

        cursor_image = QImage(w+2, h+2, QImage::Format_ARGB32_Premultiplied);;
        cursor_image.fill(Qt::black);
        {
            QPainter painter(&cursor_image);
            painter.drawImage(1, 1, (draw_tool == BRUSH)?
                brush_pixels.scaled(w, h)
              : get_low_res(pen_size, round_pen_corners).scaled(w, w)
            );
        }

        reduce_to_outline(cursor_image);
    }
}

void MapPainterWidget::paintCursor(QPainter &painter) const
{
    QPoint offset;
    switch (draw_tool)
    {
    case PEN:
    case LINE:
    case ERASER:
    case SHADER:
        offset = {pen_size / 2, pen_size / 2};
        break;
    case BRUSH:
        offset = {int(cursor_image.width() / (2*zoom)), int(cursor_image.height() / (2*zoom))};
        break;
    case SHAPE:
    case FILL:
    case PIPETTE:
    case SELECTION:
        break;
    }

    painter.drawImage((mouse_cursor - offset) * zoom, cursor_image);
}

QColor MapPainterWidget::getEffectiveDrawColor() const
{
    const QColor dark32 = {0, 0, 0, 32};
    const QColor light32 = {255, 255, 255, 32};

    if (draw_tool != SHADER)
        return draw_color;
    else if (darken)
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

    if (drag_points.length() > 1)
        painter.drawPolyline(drag_points.data(), drag_points.length());

//    painter.drawPoint(drag_points.front());
    if (drag_points.length() == 1)
        painter.drawPoint(drag_points.back());
}

void MapPainterWidget::drawLine(QPainter &painter) const
{
    setPen(painter);
    painter.drawLine(*click_origin, mouse_cursor);
}

void MapPainterWidget::drawBrush(QPainter &painter) const
{
    setPen(painter);

    const int ox = cursor_image.width() / (2*zoom);
    const int oy = cursor_image.height() / (2*zoom);

    QVector<QPoint> points;
    for (auto &[x, y]: drag_points)
        for (int j = 0; j < brush_pixels.height(); ++j)
            for (int i = 0; i < brush_pixels.width(); ++i)
                if (brush_pixels.pixelColor(i, j) == Qt::white)
                    points.push_back({x + i - ox, y + j - oy});

    painter.drawPoints(points.data(), points.length());
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

    if (drag_points.length() > 1)
        painter.drawPolyline(drag_points.data(), drag_points.length());

//    painter.drawPoint(drag_points.front());
    if (drag_points.length() == 1)
        painter.drawPoint(drag_points.back());
}

void MapPainterWidget::drawShader(QPainter &painter) const
{
    setPen(painter);
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);

    if (drag_points.length() > 1)
        painter.drawPolyline(drag_points.data(), drag_points.length());

//    painter.drawPoint(drag_points.front());
    if (drag_points.length() == 1)
        painter.drawPoint(drag_points.back());
}

void MapPainterWidget::drawSelectionPixels(QPainter &painter) const
{
    if (selection_rect && !selection_image.isNull())
        painter.drawImage(selection_rect->topLeft(), selection_image);
}

static inline void draw_outline(QPainter &painter, std::function<void(QPainter&)> fn)
{
    painter.setPen(QPen(Qt::black, 2));
    fn(painter);
    painter.setPen(QPen(Qt::white, 1));
    fn(painter);
}

void MapPainterWidget::drawSelectionOutline(QPainter &painter) const
{
    if (selection_rect)
    {
    //  QPoint rounds floats ; we don't want that
        const auto &[x, y] = selection_rect->topLeft();
        const QPoint p(int(x * zoom), int(y * zoom));
        const auto &[w, h] = selection_rect->size();
        const QSize s(int(w * zoom), int(h * zoom));

        QVector<QPoint> shown = magic_points;
        if (original_rect)
            for (int i = 0; i < shown.length(); ++i)
                shown[i] += selection_rect->topLeft();
        for (int i = 0; i < shown.length(); ++i)
                shown[i] = shown[i] * zoom;

        switch (selection_shape)
        {
        case RECTANGLE:
            draw_outline(painter, [=] (QPainter &painter) {
                painter.drawRect(QRect(p, s));
            });
            break;
        case ELLIPSE:
            draw_outline(painter, [=] (QPainter &painter) {
                painter.drawEllipse(QRect(p, s));
            });
            break;
        case MAGIC:
            draw_outline(painter, [=] (QPainter &painter) {
                if (shown.length() > 1)
                    painter.drawPolyline(shown.data(), shown.length());
                else if (shown.length() == 1)
                    painter.drawPoint(shown.front());
            });
            break;
        }
    }
}

QImage MapPainterWidget::getDrawnLayer() const
{
    QImage original = getPaintedLayer();
    if (click_origin)
    {
        QPainter painter(&original);

        switch (draw_tool)
        {
        case PEN:
            if (!drag_points.isEmpty()) drawPen(painter);
            break;
        case LINE:
            drawLine(painter); break;
        case BRUSH:
            drawBrush(painter); break;
        case SHAPE:
            drawShape(painter); break;
        case FILL:
            drawFill(original); break;
        case ERASER:
            if (!drag_points.isEmpty()) drawEraser(painter);
            break;
        case SHADER:
            if (!drag_points.isEmpty()) drawShader(painter);
            break;
        default:    //  PIPETTE, SELECTION
            break;
        }
    }

    return original;
}

void MapPainterWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    paintBackground(painter);

//  we want these contents to be pixelised
    painter.scale(zoom, zoom);
    painter.drawImage(0, 0, getDrawnLayer());
    if (draw_tool == SELECTION)
        drawSelectionPixels(painter);
    painter.resetTransform();

//  for these we don't
    if (show_grid)
        paintGrid(painter);
    if (draw_tool == SELECTION)
        drawSelectionOutline(painter);
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

static inline QImage empty(const int w, const int h)
{
    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    return img;
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
            QImage new_image = prev_id.isEmpty()?
                empty(tilesize, tilesize)
              : tileset->value(prev_id);

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

void MapPainterWidget::blitSelection()
{
    QImage original = getPaintedLayer();

    QImage drawn = original.copy();
    {
        QPainter painter(&drawn);
        painter.drawImage(selection_rect->topLeft(), selection_image);
    }

    const auto changed_pixels = get_changed_pixels(original, drawn, tilesize);
    if (changed_pixels.isEmpty())
        return;

    if (retroactive)
        handleRetroactiveDrawing(changed_pixels);
    else
        handleNonRetroactiveDrawing(changed_pixels);

    emit canCopy(false);
    selection_rect = {};
    original_rect = {};
    magic_points = {};
}

void MapPainterWidget::cutSelection()
{
    QImage original = getPaintedLayer();

    QImage drawn = original.copy();
    {
        QPainter painter(&drawn);
        painter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        painter.fillRect(*original_rect, Qt::black);
    }

    const auto changed_pixels = get_changed_pixels(original, drawn, tilesize);
    if (changed_pixels.isEmpty())
        return;

    if (retroactive)
        handleRetroactiveDrawing(changed_pixels);
    else
        handleNonRetroactiveDrawing(changed_pixels);

    emit canCopy(false);
    selection_rect = {};
    original_rect = {};
    magic_points = {};
}

void MapPainterWidget::transformSelection(const QTransform &transform)
{
    if (selection_rect && !selection_image.isNull())
    {
    //  TODO: transform also magic_points
        selection_image = selection_image.transformed(transform);
        selection_rect->setSize(selection_image.size());

        update();
    }
}

void MapPainterWidget::selectAll()
{
    selection_rect = getWidgetRect();
    selection_image = getPaintedLayer();
    original_rect = selection_rect;

    emit canCopy(true);
    update();
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

static inline std::optional<QRect> path_to_rect(const QVector<QPoint> &points)
{
    if (points.isEmpty())
        return {};

    const auto &[x0, y0] = points.front();
    int minx = x0, miny = y0, maxx = x0, maxy = y0;

    for (auto &[x, y]: points)
    {
        if (x < minx)
            minx = x;
        if (y < miny)
            miny = y;
        if (x > maxx)
            maxx = x;
        if (y > maxy)
            maxy = y;
    }

    return QRect(QPoint(minx, miny), QPoint(maxx, maxy));
}

static inline bool has_selection(const std::optional<QPoint> &offset, const std::optional<QRect> &rect)
{
    return !offset && rect && !rect->isEmpty();
}

static inline QImage get_selection_contents(const QImage &source, const SelectionShape &shape, const QVector<QPoint> &magic)
{
    QImage dest(source.size(), QImage::Format_ARGB32_Premultiplied);
    dest.fill(Qt::transparent);

    QPainter painter(&dest);
    painter.setPen(Qt::black);
    painter.setBrush(Qt::black);

    if (shape == RECTANGLE)
        painter.drawRect(dest.rect());
    else if (shape == ELLIPSE)
        painter.drawEllipse(dest.rect());
    else if (shape == MAGIC)
        painter.drawPolygon(magic.data(), magic.length());

    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.drawImage(0, 0, source);

    return dest;
}

void MapPainterWidget::handleSelectionMade()
{
    if (!(tileset && map_layers))
        return;

    if (!original_rect)
    {
        if (selection_shape == MAGIC)
            selection_rect = path_to_rect(magic_points);

        original_rect = selection_rect;

        emit canCopy(true);
        if (has_selection(move_offset, selection_rect))
        {
            QImage source = getPaintedLayer().copy(*selection_rect);
            for (int i = 0; i < magic_points.length(); ++i)
                magic_points[i] = magic_points[i] - selection_rect->topLeft();
//            magic_points.push_back(magic_points.front());
            QImage dest = get_selection_contents(source, selection_shape, magic_points);

            selection_image = dest;
        }
    }

    move_offset = {};
}

static inline QRect rect_from(const QPoint &p1, const QPoint &p2)
{
    return {
        qMin(p1.x(), p2.x()), qMin(p1.y(), p2.y()),
        qAbs(p1.x() - p2.x()), qAbs(p1.y() - p2.y())
    };
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
            drag_points.push_back(mouse_cursor);
            break;
        case PIPETTE:
            emit colorChanged(getColorAt(mouse_cursor));
            break;
        case SELECTION:
            if (move_offset)
                selection_rect->moveTo(mouse_cursor - *move_offset);
            else if (selection_shape != MAGIC)
                selection_rect = rect_from(*click_origin, mouse_cursor);
            else
                magic_points.push_back(mouse_cursor);
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

        if (draw_tool == SELECTION)
        {
            if (selection_rect && selection_rect->contains(mouse_cursor))
            {
                move_offset = mouse_cursor - selection_rect->topLeft();
            }
            else
            {
                if (selection_rect)
                    blitSelection();

            //  1x1 would create a selection even if we just clicked
                selection_rect = QRect(mouse_cursor, QSize(0, 0));
                magic_points = {mouse_cursor};
                selection_image = {};
                original_rect = {};
            }
        }
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
        if (draw_tool != SELECTION)
            handleDrawChanges();
        else
            handleSelectionMade();

        click_origin = {};
        drag_points = {};
        move_offset = {};
    }

    if (event->button() == Qt::RightButton)
        right_click = false;

    update();
}
