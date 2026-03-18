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

//  doesn't matter if the tiles_order is simple or auto tiles
class TilesOrderCommand
{
public:
    TilesOrderCommand(QWeakPointer<Names> tiles_order): tiles_order_ptr{tiles_order} {}
    QSharedPointer<Names> lockTilesOrder() { return lock_ptr(tiles_order_ptr); }

private:
    QWeakPointer<Names> tiles_order_ptr;
};

template <typename T>
class TilesCommand
{
public:
    TilesCommand(QWeakPointer<T> tiles): tiles_ptr{tiles} {}
    QSharedPointer<T> lockTiles() { return lock_ptr(tiles_ptr); }

private:
    QWeakPointer<T> tiles_ptr;
};

class MapLayersCommand
{
public:
    MapLayersCommand(QWeakPointer<MapLayers> map_layers): map_layers_ptr{map_layers} {}
    QSharedPointer<MapLayers> lockMapLayers() { return lock_ptr(map_layers_ptr); }

private:
    QWeakPointer<MapLayers> map_layers_ptr;
};

class MoveTileCommand final: public QUndoCommand, public TilesOrderCommand
{
public:
    MoveTileCommand(QWeakPointer<Names> ptr, const int origin, const int target):
        QUndoCommand(), TilesOrderCommand(ptr), origin{origin}, target{target}
    {}

    void undo() final override
    {
        lockTilesOrder()->insert(origin, lockTilesOrder()->takeAt(target));
    }

    void redo() final override
    {
        lockTilesOrder()->insert(target, lockTilesOrder()->takeAt(origin));
    }

private:
    int origin, target;
};

class SwapTilesCommand final: public QUndoCommand, public TilesOrderCommand
{
public:
    SwapTilesCommand(QWeakPointer<Names> ptr, const int index1, const int index2):
        QUndoCommand(), TilesOrderCommand(ptr), index1{index1}, index2{index2}
    {}

    void undo() final override
    {
        lockTilesOrder()->swapItemsAt(index1, index2);
    }

    void redo() final override
    {
        lockTilesOrder()->swapItemsAt(index1, index2);
    }

private:
    int index1, index2;
};

template <typename T>
class AddTileCommand: public QUndoCommand, public TilesOrderCommand, public TilesCommand<Tileset<T>>
{
public:
    AddTileCommand(QWeakPointer<Names> tiles_order, QWeakPointer<Tileset<T>> tiles, const T &tile):
        QUndoCommand(), TilesOrderCommand(tiles_order), TilesCommand<Tileset<T>>(tiles),
        index{0}, added_tile{tile}, added_ref{}
    {
        index = lockTilesOrder()->length();
        const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        added_ref.name = id;
    }

    void undo() final override
    {
        lockTilesOrder()->remove(index);
        TilesCommand<Tileset<T>>::lockTiles()->remove(added_ref.name);
    }

    void redo() final override
    {
        lockTilesOrder()->push_back(added_ref.name);
        TilesCommand<Tileset<T>>::lockTiles()->insert(added_ref.name, added_tile);
    }

private:
    int index;
    T added_tile;

protected:
    TileReference added_ref;
};

class AddSimpleTileCommand final: public AddTileCommand<SimpleTile>
{
public:
    AddSimpleTileCommand(QWeakPointer<Names> tiles_order, QWeakPointer<SimpleTiles> tiles, const SimpleTile &tile):
        AddTileCommand(tiles_order, tiles, tile)
    {
        added_ref.autotile = false;
    }
};

class AddAutoTileCommand final: public AddTileCommand<AutoTile>
{
public:
    AddAutoTileCommand(QWeakPointer<Names> tiles_order, QWeakPointer<AutoTiles> tiles, const AutoTile &tile):
        AddTileCommand(tiles_order, tiles, tile)
    {
        added_ref.autotile = true;
    //  see Types.hpp for why this represents a single isolated tile
        added_ref.orientation = {0, 3, 12, 15};
    }
};

struct MapLayersCoordinates
{
    int i, j, k;

    bool operator==(const MapLayersCoordinates &other) const
    {
        return (i == other.i) && (j == other.j) && (k == other.k);
    }
};

static inline uint qHash(const MapLayersCoordinates &coords, const uint seed = 0)
{
    return seed ^ (
        qHash(coords.i, seed) * 31
      + qHash(coords.j, seed) * 37
      + qHash(coords.k, seed) * 41
    );
}

template <typename T>
class RemoveTileCommand final: public QUndoCommand, public TilesOrderCommand, public TilesCommand<Tileset<T>>, public MapLayersCommand
{
public:
    RemoveTileCommand(QWeakPointer<Names> tiles_order, QWeakPointer<Tileset<T>> simple_tiles, QWeakPointer<MapLayers> map_layers, const QString &id):
        QUndoCommand(), TilesOrderCommand(tiles_order), TilesCommand<Tileset<T>>(simple_tiles), MapLayersCommand(map_layers),
        index{0}, id{id}, tile{}
    {
        index = lockTilesOrder()->indexOf(id);
        tile = TilesCommand<Tileset<T>>::lockTiles()->value(id);

        auto layers = *lockMapLayers();
        for (int k = 0; k < layers.length(); ++k)
            for (int j = 0; j < layers.at(k).length(); ++j)
                for (int i = 0; i < layers.at(k).at(j).length(); ++i)
                    if (layers.at(k).at(j).at(i).name == id)
                        affected_tiles.insert({i, j, k}, layers.at(k).at(j).at(i));
    }

    void undo() final override
    {
        lockTilesOrder()->insert(index, id);
        TilesCommand<Tileset<T>>::lockTiles()->insert(id, tile);

        for (auto &[i, j, k]: affected_tiles.keys())
            (*lockMapLayers())[k][j][i] = affected_tiles[{i, j, k}];
    }

    void redo() final override
    {
        lockTilesOrder()->remove(index);
        TilesCommand<Tileset<T>>::lockTiles()->remove(id);

        for (auto &[i, j, k]: affected_tiles.keys())
            (*lockMapLayers())[k][j][i] = {};
    }

private:
    int index;
    QString id;
    T tile;
    QHash<MapLayersCoordinates, TileReference> affected_tiles;
};

template <typename T, class Frame>
class AddFrameCommand final: public QUndoCommand, public TilesCommand<Tileset<T>>
{
public:
    AddFrameCommand(QWeakPointer<Tileset<T>> tiles, const QString &id, const int index, const Frame &frame):
        QUndoCommand(), TilesCommand<Tileset<T>>(tiles),
        id{id}, index{index}, frame{frame}
    {}

    void undo() final override
    {
        (*TilesCommand<Tileset<T>>::lockTiles())[id].frames.remove(index);
    }

    void redo() final override
    {
        (*TilesCommand<Tileset<T>>::lockTiles())[id].frames.insert(index, frame);
    }

private:
    QString id;
    int index;
    Frame frame;
};

template <typename T, class Frame>
class RemoveFrameCommand final: public QUndoCommand, public TilesCommand<Tileset<T>>
{
public:
    RemoveFrameCommand(QWeakPointer<Tileset<T>> tiles, const QString &id, const int index):
        QUndoCommand(), TilesCommand<Tileset<T>>(tiles),
        id{id}, index{index}, removed{}
    {
        Q_ASSERT(TilesCommand<Tileset<T>>::lockTiles()->contains(id));
        const auto &frames = TilesCommand<Tileset<T>>::lockTiles()->value(id).frames;
        Q_ASSERT(frames.length() > 1);
        Q_ASSERT(0 <= index && index < frames.length());
        removed = frames.at(index);
    }

    void undo() final override
    {
        (*TilesCommand<Tileset<T>>::lockTiles())[id].frames.insert(index, removed);
    }

    void redo() final override
    {
        (*TilesCommand<Tileset<T>>::lockTiles())[id].frames.remove(index);
    }

private:
    QString id;
    int index;
    Frame removed;
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

void TilesetViewWidget::addSimpleTile(const SimpleTile &simple_tile)
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!simple_tiles_order.isNull());
    Q_ASSERT(!simple_tiles.isNull());

    undo_stack->push(new AddSimpleTileCommand(simple_tiles_order, simple_tiles, simple_tile));
}

void TilesetViewWidget::addAutoTile(const AutoTile &autotile)
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!autotiles_order.isNull());
    Q_ASSERT(!autotiles.isNull());

    undo_stack->push(new AddAutoTileCommand(autotiles_order, autotiles, autotile));
}

void TilesetViewWidget::removeTiles(const QVector<TileReference> &tiles)
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!simple_tiles_order.isNull());
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(!map_layers.isNull());
    for (auto &ref: tiles)
        Q_ASSERT(simple_tiles->contains(ref.name) || autotiles->contains(ref.name));

    undo_stack->beginMacro("Remove Tiles");
    for (auto &ref: tiles)
        if (ref.autotile)
            undo_stack->push(new RemoveTileCommand<AutoTile>(autotiles_order, autotiles, map_layers, ref.name));
        else
            undo_stack->push(new RemoveTileCommand<SimpleTile>(simple_tiles_order, simple_tiles, map_layers, ref.name));
    undo_stack->endMacro();

    emit tilesRemoved();
}

static inline bool is_1x1(const SelectedTiles &selected)
{
    return (selected.length() == 1) && (selected[0].length() == 1);
}

void TilesetViewWidget::addFrame(const int index, const QImage &frame)
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(!selected_tiles.isNull());
    Q_ASSERT(is_1x1(*selected_tiles));

    const auto ref = selected_tiles->at(0).at(0);
    Q_ASSERT(!ref.autotile);
    Q_ASSERT(simple_tiles->contains(ref.name));

    const auto &frames = simple_tiles->value(ref.name).frames;
//  +1 because we may want to add at the end
    Q_ASSERT(0 <= index && index < frames.length() + 1);

    undo_stack->push(new AddFrameCommand<SimpleTile, QImage>(simple_tiles, ref.name, index, frame));
}

void TilesetViewWidget::addFrame(const int index, const AutoTile::Frame &frame)
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!autotiles.isNull());
    Q_ASSERT(!selected_tiles.isNull());
    Q_ASSERT(is_1x1(*selected_tiles));

    const auto ref = selected_tiles->at(0).at(0);
    Q_ASSERT(ref.autotile);
    Q_ASSERT(autotiles->contains(ref.name));

    const auto &frames = autotiles->value(ref.name).frames;
//  +1 because we may want to add at the end
    Q_ASSERT(0 <= index && index < frames.length() + 1);

    undo_stack->push(new AddFrameCommand<AutoTile, AutoTile::Frame>(autotiles, ref.name, index, frame));
}

void TilesetViewWidget::removeFrame(const int index)
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!selected_tiles.isNull());
    Q_ASSERT(is_1x1(*selected_tiles));
    const auto ref = selected_tiles->at(0).at(0);

    if (ref.autotile)
    {
        Q_ASSERT(!autotiles.isNull());
        const auto &frames = autotiles->value(ref.name).frames;
        Q_ASSERT(0 <= index && index < frames.length());

        undo_stack->push(new RemoveFrameCommand<AutoTile, AutoTile::Frame>(autotiles, ref.name, index));
    }
    else
    {
        Q_ASSERT(!simple_tiles.isNull());
        const auto &frames = simple_tiles->value(ref.name).frames;
        Q_ASSERT(0 <= index && index < frames.length());

        undo_stack->push(new RemoveFrameCommand<SimpleTile, QImage>(simple_tiles, ref.name, index));
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
        //  see Types.hpp for why this is an isolated autotile
            return TileReference{autotiles_order->at(k-1), true, {0, 3, 12, 15}};
        else
            return {};
    }
    else
    {
        Q_ASSERT(!simple_tiles_order.isNull());
        const int n_simple = simple_tiles_order->length();

        const int k = i + (j - h_auto) * n_columns;

        if (k < n_simple)
            return TileReference{simple_tiles_order->at(k), false, {}};
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

void TilesetViewWidget::paintAutoTiles(QPainter &painter)
{
    Q_ASSERT(!autotiles_order.isNull());

//  see Types.hpp for why this represents an isolated autotile
    const Orientation isolated = {0, 3, 12, 15};

//  copying
    Names displayed = *autotiles_order;
    if (click_origin || right_click_origin)
    {
        const auto p = right_click_origin? *right_click_origin : *click_origin;

        if (const auto origin = toRef(divide(p, tilesize)); origin && *origin)
            if (const auto target = toRef(divide(mouse_cursor, tilesize)); target && *target)
                if (origin->autotile && target->autotile)
                    apply_changes(
                        *autotiles_order, displayed,
                        origin->name, target->name,
                        drag_mode, bool(click_origin), bool(right_click_origin)
                    );
    }

    for (int i = 0; i < displayed.length(); ++i)
    {
        const QString id = displayed[i];
        const QPoint p((i+1) % n_columns, (i+1) / n_columns);

        Q_ASSERT(autotiles->contains(id));
        const auto &frames = (*autotiles)[id].frames;
        const int n = frames.length();
        painter.drawImage(p * tilesize, frames[qMin(current_frame, n-1)].genTile(isolated));
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

    for (int i = 0; i < displayed.length(); ++i)
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
            unique_refs.insert({ref.name, ref.autotile, {}});

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
                    undo_stack->push(new MoveTileCommand(autotiles_order, i, j));
                else if (drag_mode == SWAP_MODE && click_origin)
                    undo_stack->push(new SwapTilesCommand(autotiles_order, i, j));
                else if (drag_mode == SELECTION_MODE && right_click_origin)
                    undo_stack->push(new MoveTileCommand(autotiles_order, i, j));
            }
            else if (!origin->autotile && !target->autotile)
            {
                const int i = simple_tiles_order->indexOf(origin->name);
                const int j = simple_tiles_order->indexOf(target->name);

                if (drag_mode == MOVE_MODE && click_origin)
                    undo_stack->push(new MoveTileCommand(simple_tiles_order, i, j));
                else if (drag_mode == SWAP_MODE && click_origin)
                    undo_stack->push(new SwapTilesCommand(simple_tiles_order, i, j));
                else if (drag_mode == SELECTION_MODE && right_click_origin)
                    undo_stack->push(new MoveTileCommand(simple_tiles_order, i, j));
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
