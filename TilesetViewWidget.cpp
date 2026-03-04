#include "TilesetViewWidget.hpp"

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

class SimpleTilesOrderCommand
{
public:
    SimpleTilesOrderCommand(QWeakPointer<Names> simple_tiles_order): simple_tiles_order_ptr{simple_tiles_order} {}
    QSharedPointer<Names> lockSimpleTilesOrder() { return lock_ptr(simple_tiles_order_ptr); }

private:
    QWeakPointer<Names> simple_tiles_order_ptr;
};

class AutoTilesOrderCommand
{
public:
    AutoTilesOrderCommand(QWeakPointer<Names> autotiles_order): autotiles_order_ptr{autotiles_order} {}
    QSharedPointer<Names> lockAutoTilesOrder() { return lock_ptr(autotiles_order_ptr); }

private:
    QWeakPointer<Names> autotiles_order_ptr;
};

class SimpleTilesCommand
{
public:
    SimpleTilesCommand(QWeakPointer<SimpleTiles> simple_tiles): simple_tiles_ptr{simple_tiles} {}
    QSharedPointer<SimpleTiles> lockSimpleTiles() { return lock_ptr(simple_tiles_ptr); }

private:
    QWeakPointer<SimpleTiles> simple_tiles_ptr;
};

class MapLayersCommand
{
public:
    MapLayersCommand(QWeakPointer<MapLayers> map_layers): map_layers_ptr{map_layers} {}
    QSharedPointer<MapLayers> lockMapLayers() { return lock_ptr(map_layers_ptr); }

private:
    QWeakPointer<MapLayers> map_layers_ptr;
};

class MoveSimpleTileCommand final: public QUndoCommand, public SimpleTilesOrderCommand
{
public:
    MoveSimpleTileCommand(QWeakPointer<Names> ptr, const int origin, const int target):
        QUndoCommand(), SimpleTilesOrderCommand(ptr), origin{origin}, target{target}
    {}

    void undo() final override
    {
        lockSimpleTilesOrder()->insert(origin, lockSimpleTilesOrder()->takeAt(target));
    }

    void redo() final override
    {
        lockSimpleTilesOrder()->insert(target, lockSimpleTilesOrder()->takeAt(origin));
    }

private:
    int origin, target;
};

class MoveAutoTileCommand final: public QUndoCommand, public AutoTilesOrderCommand
{
public:
    MoveAutoTileCommand(QWeakPointer<Names> ptr, const int origin, const int target):
        QUndoCommand(), AutoTilesOrderCommand(ptr), origin{origin}, target{target}
    {}

    void undo() final override
    {
        lockAutoTilesOrder()->insert(origin, lockAutoTilesOrder()->takeAt(target));
    }

    void redo() final override
    {
        lockAutoTilesOrder()->insert(target, lockAutoTilesOrder()->takeAt(origin));
    }

private:
    int origin, target;
};

class SwapSimpleTilesCommand final: public QUndoCommand, public SimpleTilesOrderCommand
{
public:
    SwapSimpleTilesCommand(QWeakPointer<Names> ptr, const int index1, const int index2):
        QUndoCommand(), SimpleTilesOrderCommand(ptr), index1{index1}, index2{index2}
    {}

    void undo() final override
    {
        lockSimpleTilesOrder()->swapItemsAt(index1, index2);
    }

    void redo() final override
    {
        lockSimpleTilesOrder()->swapItemsAt(index1, index2);
    }

private:
    int index1, index2;
};

class SwapAutoTilesCommand final: public QUndoCommand, public AutoTilesOrderCommand
{
public:
    SwapAutoTilesCommand(QWeakPointer<Names> ptr, const int index1, const int index2):
        QUndoCommand(), AutoTilesOrderCommand(ptr), index1{index1}, index2{index2}
    {}

    void undo() final override
    {
        lockAutoTilesOrder()->swapItemsAt(index1, index2);
    }

    void redo() final override
    {
        lockAutoTilesOrder()->swapItemsAt(index1, index2);
    }

private:
    int index1, index2;
};

class AddSimpleTileCommand final: public QUndoCommand, public SimpleTilesOrderCommand, public SimpleTilesCommand
{
public:
    AddSimpleTileCommand(QWeakPointer<Names> tiles_order, QWeakPointer<SimpleTiles> simple_tiles, const SimpleTile &tile):
        QUndoCommand(), SimpleTilesOrderCommand(tiles_order), SimpleTilesCommand(simple_tiles),
        index{0}, added_ref{}, added_tile{tile}
    {
        index = lockSimpleTilesOrder()->length();
        const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        added_ref = {id, false};
    }

    void undo() final override
    {
        lockSimpleTilesOrder()->remove(index);
        lockSimpleTiles()->remove(added_ref.name);
    }

    void redo() final override
    {
        lockSimpleTilesOrder()->push_back(added_ref.name);
        lockSimpleTiles()->insert(added_ref.name, added_tile);
    }

private:
    int index;
    TileReference added_ref;
    SimpleTile added_tile;
};

class RemoveSimpleTileCommand final: public QUndoCommand, public SimpleTilesOrderCommand, public SimpleTilesCommand, public MapLayersCommand
{
public:
    struct Coordinates
    {
        int i, j, k;

        bool operator==(const Coordinates &other) const
        {
            return i == other.i && j == other.j && k == other.k;
        }
    };

    RemoveSimpleTileCommand(QWeakPointer<Names> tiles_order, QWeakPointer<SimpleTiles> simple_tiles, QWeakPointer<MapLayers> map_layers, const TileReference &id):
        QUndoCommand(), SimpleTilesOrderCommand(tiles_order), SimpleTilesCommand(simple_tiles), MapLayersCommand(map_layers),
        index{0}, tile_reference{id}, tile{}
    {
        index = lockSimpleTilesOrder()->indexOf(tile_reference.name);
        tile = lockSimpleTiles()->value(tile_reference.name);

        auto layers = *lockMapLayers();
        for (int k = 0; k < layers.length(); ++k)
            for (int j = 0; j < layers.at(k).length(); ++j)
                for (int i = 0; i < layers.at(k).at(j).length(); ++i)
                    if (layers.at(k).at(j).at(i) == id)
                        affected_tiles.insert({i, j, k}, layers.at(k).at(j).at(i));
    }

    void undo() final override
    {
        lockSimpleTilesOrder()->insert(index, tile_reference.name);
        lockSimpleTiles()->insert(tile_reference.name, tile);

        for (auto &[i, j, k]: affected_tiles.keys())
            (*lockMapLayers())[k][j][i] = affected_tiles[{i, j, k}];
    }

    void redo() final override
    {
        lockSimpleTilesOrder()->remove(index);
        lockSimpleTiles()->remove(tile_reference.name);

        for (auto &[i, j, k]: affected_tiles.keys())
            (*lockMapLayers())[k][j][i] = {};
    }

private:
    int index;
    TileReference tile_reference;
    SimpleTile tile;
    QHash<Coordinates, TileReference> affected_tiles;
};

static inline uint qHash(const RemoveSimpleTileCommand::Coordinates &coords, const uint seed = 0)
{
    return seed ^ (
        qHash(coords.i, seed) * 31
      + qHash(coords.j, seed) * 37
      + qHash(coords.k, seed) * 41
    );
}

class AddSimpleTileFrameCommand final: public QUndoCommand, public SimpleTilesCommand
{
public:
    AddSimpleTileFrameCommand(QWeakPointer<SimpleTiles> simple_tiles, const TileReference &id, const int index, const QImage &frame):
        QUndoCommand(), SimpleTilesCommand(simple_tiles), tile_reference{id}, frame_index{index}, frame_image{frame}
    {}

    void undo() final override
    {
        (*lockSimpleTiles())[tile_reference.name].frames.remove(frame_index);
    }

    void redo() final override
    {
        (*lockSimpleTiles())[tile_reference.name].frames.insert(frame_index, frame_image);
    }

private:
    TileReference tile_reference;
    int frame_index;
    QImage frame_image;
};

class RemoveSimpleTileFrameCommand final: public QUndoCommand, public SimpleTilesCommand
{
public:
    RemoveSimpleTileFrameCommand(QWeakPointer<SimpleTiles> simple_tiles, const TileReference &ref, const int index):
        QUndoCommand(), SimpleTilesCommand(simple_tiles), tile_reference{ref}, frame_index{index}, removed_image{}
    {
        Q_ASSERT(lockSimpleTiles()->contains(ref.name));
        const auto &frames = lockSimpleTiles()->value(ref.name).frames;
        Q_ASSERT(frames.length() > 1);
        Q_ASSERT(0 <= index && index < frames.length());
        removed_image = frames.at(index);
    }

    void undo() final override
    {
        (*lockSimpleTiles())[tile_reference.name].frames.insert(frame_index, removed_image);
    }

    void redo() final override
    {
        (*lockSimpleTiles())[tile_reference.name].frames.remove(frame_index);
    }

private:
    TileReference tile_reference;
    int frame_index;
    QImage removed_image;
};

TilesetViewWidget::TilesetViewWidget(QWidget *parent):
    EditorWidget(parent),
    n_columns{8}, drag_mode{SELECTION_MODE},
    mouse_cursor{}, click_origin{}, right_click_origin{}
{
    resize();
}

void TilesetViewWidget::setDragMode(const int index)
{
    Q_ASSERT(0 <= index && index < 3);
    drag_mode = DragMode(index);
}

void TilesetViewWidget::resize()
{
    const int n_auto = autotiles_order? autotiles_order->length() : 0;
    const int n_simple = simple_tiles_order? simple_tiles_order->length() : 0;

//  empty tile => +1
    const int h_auto = qCeil(double(n_auto + 1) / n_columns);
    const int h_simple = qCeil(double(n_simple) / n_columns);

    grid_aspect = {n_columns, h_auto + h_simple};

    EditorWidget::resize();
}

void TilesetViewWidget::addSimpleTiles(const QVector<SimpleTile> &images, const bool undoable)
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!simple_tiles_order.isNull());
    Q_ASSERT(!simple_tiles.isNull());

    if (undoable)
    {
        undo_stack->beginMacro("Add Tiles");
        for (auto &pixels: images)
            undo_stack->push(new AddSimpleTileCommand(simple_tiles_order, simple_tiles, pixels));
        undo_stack->endMacro();
    }
    else
    {
        for (auto &pixels: images)
        {
            const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            simple_tiles_order->push_back(id);
            simple_tiles->insert(id, pixels);
        }
    }

    update();
}

void TilesetViewWidget::removeTiles(const QVector<TileReference> &tiles)
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!simple_tiles_order.isNull());
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(!map_layers.isNull());
    for (auto &ref: tiles)
        Q_ASSERT(simple_tiles->contains(ref.name));

    undo_stack->beginMacro("Remove Tiles");
    for (auto &ref: tiles)
        if (ref.autotile)
        {}
        else
            undo_stack->push(new RemoveSimpleTileCommand(simple_tiles_order, simple_tiles, map_layers, ref));
    undo_stack->endMacro();

    emit tilesRemoved();
}

static inline bool is_1x1(const SelectedTiles &selected)
{
    return (selected.length() == 1) && (selected[0].length() == 1);
}

void TilesetViewWidget::addFrames(const QHash<int, QImage> &added)
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(!selected_tiles.isNull());
    Q_ASSERT(is_1x1(*selected_tiles));
    const auto ref = selected_tiles->at(0).at(0);

    if (ref.autotile)
    {}
    else
    {
        Q_ASSERT(simple_tiles->contains(ref.name));
        const auto &frames = simple_tiles->value(ref.name).frames;
        for (auto &index: added.keys())
        //  +1 because we may want to add at the end
            Q_ASSERT(0 <= index && index < frames.length() + 1);

        undo_stack->beginMacro("Add Frames");
        for (auto &index: added.keys())
            undo_stack->push(new AddSimpleTileFrameCommand(simple_tiles, ref, index, added[index]));
        undo_stack->endMacro();
    }
}

void TilesetViewWidget::removeFrames(const QVector<int> &indexes)
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(!selected_tiles.isNull());
    Q_ASSERT(is_1x1(*selected_tiles));
    const auto ref = selected_tiles->at(0).at(0);

    if (ref.autotile)
    {}
    else
    {
        Q_ASSERT(simple_tiles->contains(ref.name));
        const auto &frames = simple_tiles->value(ref.name).frames;
        for (auto &index: indexes)
            Q_ASSERT(0 <= index && index < frames.length() + 1);

        undo_stack->beginMacro("Remove Frames");
        for (auto &index: indexes)
            undo_stack->push(new RemoveSimpleTileFrameCommand(simple_tiles, ref, index));
        undo_stack->endMacro();
    }
}

std::optional<TileReference> TilesetViewWidget::toRef(const QPoint &ij) const
{
    const auto &[i, j] = ij;

    Q_ASSERT(!autotiles_order.isNull());
    const int n_auto = autotiles_order->length();
    const int h_auto = qCeil(double(n_auto + 1) / n_columns);

    if (j < h_auto)
    {
        const int k = i + j * n_columns;

        if (k == 0)
            return TileReference{};
        else if (k-1 < n_auto)
            return TileReference{autotiles_order->at(k-1), true};
        else
            return {};
    }
    else
    {
        Q_ASSERT(!simple_tiles_order.isNull());
        const int n_simple = simple_tiles_order->length();

        const int k = i + (j - h_auto) * n_columns;

        if (k < n_simple)
            return TileReference{simple_tiles_order->at(k), false};
        else
            return {};
    }
}

//  QPoint's division rounds ; we DON'T want that
static inline QPoint divide(const QPoint &p, const double a)
{
    return {int(p.x() / a), int(p.y() / a)};
}

static inline void apply_changes(const Names &original, Names &copy, const QString &origin, const QString &target, const DragMode mode, const bool left, const bool right)
{
    const int i = original.indexOf(origin);
    const int j = original.indexOf(target);

    if (mode == MOVE_MODE && left)
        copy.insert(j, copy.takeAt(i));
    else if (mode == SWAP_MODE && left)
        copy.swapItemsAt(i, j);
    else if (mode == SELECTION_MODE && right)
        copy.insert(j, copy.takeAt(i));
}

void TilesetViewWidget::paintAutoTiles(QPainter &/*painter*/)
{
    Q_ASSERT(!autotiles_order.isNull());

    const int n_auto = autotiles_order->length();

    for (int i = 0; i < n_auto; ++i)
    {
        const QString id = autotiles_order->at(i);
        const QPoint p((i+1) % n_columns, (i+1) / n_columns);
    }
}

void TilesetViewWidget::paintSimpleTiles(QPainter &painter)
{
    Q_ASSERT(!simple_tiles_order.isNull());
    Q_ASSERT(!simple_tiles.isNull());

//  copying
    Names displayed = *simple_tiles_order;
    if (click_origin || right_click_origin)
    {
        const auto p = right_click_origin? *right_click_origin : *click_origin;

        if (const auto origin = toRef(divide(p, tilesize)); origin && *origin)
            if (const auto target = toRef(divide(mouse_cursor, tilesize)); target && *target)
                if (!origin->autotile && !target->autotile)
                    apply_changes(
                        *simple_tiles_order, displayed,
                        origin->name, target->name,
                        drag_mode, bool(click_origin), bool(right_click_origin)
                    );
    }

    const int n_auto = autotiles_order->length();
    const int h_auto = qCeil(double(n_auto + 1) / n_columns);

    const int n_simple = displayed.length();

    for (int i = 0; i < n_simple; ++i)
    {
        const QString id = displayed[i];
        const QPoint p(i % n_columns, h_auto + i / n_columns);

        Q_ASSERT(simple_tiles->contains(id));
        const auto &frames = (*simple_tiles)[id].frames;
        const int n = frames.length();
        painter.drawImage(p * tilesize, frames[qMin(current_frame, n-1)]);
    }
}

static inline void draw_selection(QPainter &painter, const QPoint &top_left, const int size)
{
    const QColor white64 = {255, 255, 255, 64};

    const QRect outer = {top_left, QSize(size-1, size-1)};
    painter.fillRect(outer, white64);
    painter.setPen(Qt::black);
    painter.drawRect(outer);

    const QRect inner = {top_left + QPoint(1, 1), QSize(size-3, size-3)};
    painter.setPen(Qt::white);
    painter.drawRect(inner);
}

void TilesetViewWidget::paintSelectionCursors(QPainter &painter)
{
    Q_ASSERT(!selected_tiles.isNull());
    Q_ASSERT(!autotiles_order.isNull());
    Q_ASSERT(!simple_tiles_order.isNull());

//  more readable and concise, with hardly any performances drop
    QSet<TileReference> unique_refs;
    for (auto &row: *selected_tiles)
        for (auto &ref: row)
            unique_refs.insert(ref);

    for (auto &ref: unique_refs)
    {
        if (!ref)
        {
            draw_selection(painter, {0, 0}, tilesize);
        }
        else if (ref.autotile)
        {
            const int index = autotiles_order->indexOf(ref.name);
            Q_ASSERT(index >= 0);

            const QPoint p((index+1) % n_columns, (index+1) / n_columns);
            draw_selection(painter, p * tilesize, tilesize);
        }
        else
        {
            const int index = simple_tiles_order->indexOf(ref.name);
            Q_ASSERT(index >= 0);

            const int n_auto = autotiles_order->length();
            const int h_auto = qCeil(double(n_auto + 1) / n_columns);

            const QPoint p(index % n_columns, h_auto + index / n_columns);
            draw_selection(painter, p * tilesize, tilesize);
        }
    }
}

void TilesetViewWidget::paintCursor(QPainter &painter)
{
    const QPoint p = divide(mouse_cursor, tilesize);
    const QRect draw_rect = {p * tilesize, QSize(tilesize, tilesize)};

    const QColor white32 = {255, 255, 255, 32};
    const QColor red32 = {255, 0, 0, 32};

    painter.fillRect(draw_rect, toRef(p)? white32 : red32);
}

void TilesetViewWidget::paintSelectionRect(QPainter &painter)
{
    if (click_origin)
    {
        const QRect selection = asLocalRect(*click_origin, mouse_cursor);
        const QPoint top_left = selection.topLeft() * tilesize;
        const QSize size = selection.size() * tilesize;
    //  drawRect has a weird way of overshooting by one pixel...
        const QRect draw_rect(top_left, size - QSize(1, 1));

        painter.setPen(Qt::white);
        painter.drawRect(draw_rect);
        const QColor white64 = {255, 255, 255, 64};
        painter.fillRect(draw_rect, white64);
    }
}

void TilesetViewWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    paintBackground(painter);

    paintAutoTiles(painter);
    paintSimpleTiles(painter);

    paintGrid(painter);
    paintSelectionCursors(painter);
    paintCursor(painter);

    if (drag_mode == SELECTION_MODE)
        paintSelectionRect(painter);
}

void TilesetViewWidget::handleTilesSelected()
{
    Q_ASSERT(!autotiles_order.isNull());
    Q_ASSERT(!simple_tiles_order.isNull());
    Q_ASSERT(!selected_tiles.isNull());
    Q_ASSERT(click_origin.has_value());

    const QRect selection = asLocalRect(*click_origin, mouse_cursor);
    const auto &[x1, y1] = selection.topLeft();
    const auto &[x2, y2] = selection.bottomRight();

    selected_tiles->clear();
    for (int j = y1; j <= y2; ++j)
    {
        selected_tiles->push_back({});

        for (int i = x1; i <= x2; ++i)
            if (const auto ref = toRef({i, j}))
                selected_tiles->back().push_back(*ref);
    }

    emit selectedChanged();
}

void TilesetViewWidget::handleTileModifications()
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!simple_tiles_order.isNull());

    const QPoint p = right_click_origin? *right_click_origin : *click_origin;

    if (const auto origin = toRef(divide(p, tilesize)); origin && *origin)
    {
        if (const auto target = toRef(divide(mouse_cursor, tilesize)); target && *target)
        {
            if (origin->autotile && target->autotile)
            {
                const int i = autotiles_order->indexOf(origin->name);
                const int j = autotiles_order->indexOf(target->name);

                if (drag_mode == MOVE_MODE && click_origin)
                    undo_stack->push(new MoveAutoTileCommand(autotiles_order, i, j));
                else if (drag_mode == SWAP_MODE && click_origin)
                    undo_stack->push(new SwapAutoTilesCommand(autotiles_order, i, j));
                else if (drag_mode == SELECTION_MODE && right_click_origin)
                    undo_stack->push(new MoveAutoTileCommand(autotiles_order, i, j));
            }
            else if (!origin->autotile && !target->autotile)
            {
                const int i = simple_tiles_order->indexOf(origin->name);
                const int j = simple_tiles_order->indexOf(target->name);

                if (drag_mode == MOVE_MODE && click_origin)
                    undo_stack->push(new MoveSimpleTileCommand(simple_tiles_order, i, j));
                else if (drag_mode == SWAP_MODE && click_origin)
                    undo_stack->push(new SwapSimpleTilesCommand(simple_tiles_order, i, j));
                else if (drag_mode == SELECTION_MODE && right_click_origin)
                    undo_stack->push(new MoveSimpleTileCommand(simple_tiles_order, i, j));
            }
        }
    }
}

static inline int clamp(const int lower, const int x, const int upper)
{
    if (x < lower)
        return lower;
    else if (x > upper)
        return upper;
    else
        return x;
}

static inline QPoint clamp(const QRect &limits, const QPoint &p)
{
    return {
        clamp(limits.left(), p.x(), limits.right()),
        clamp(limits.top(), p.y(), limits.bottom())
    };
}

void TilesetViewWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouse_cursor = clamp(getWidgetRect(), event->pos());

    update();
}

void TilesetViewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        if (getWidgetRect().contains(event->pos()))
            click_origin = event->pos();

    if (event->button() == Qt::RightButton)
        if (getWidgetRect().contains(event->pos()))
            right_click_origin = event->pos();

    update();
}

void TilesetViewWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (drag_mode == SELECTION_MODE)
            handleTilesSelected();
        else
            handleTileModifications();

        click_origin = {};
    }

    if (event->button() == Qt::RightButton)
    {
        if (drag_mode == SELECTION_MODE)
            handleTileModifications();
        right_click_origin = {};
    }

    update();
}
