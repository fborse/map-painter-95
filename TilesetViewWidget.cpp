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

class AddTileCommand final: public QUndoCommand, public TilesOrderCommand, public TilesetCommand
{
public:
    AddTileCommand(QWeakPointer<Names> tiles_order, QWeakPointer<Tileset> tileset, const QImage &pixels):
        QUndoCommand(), TilesOrderCommand(tiles_order), TilesetCommand(tileset),
        index{0}, id{}, image{pixels}
    {
        index = lockTilesOrder()->length();
        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    void undo() final override
    {
        lockTilesOrder()->remove(index);
        lockTileset()->remove(id);
    }

    void redo() final override
    {
        lockTilesOrder()->push_back(id);
        lockTileset()->insert(id, image);
    }

private:
    int index;
    TileReference id;
    QImage image;
};

class RemoveTileCommand final: public QUndoCommand, public TilesOrderCommand, public TilesetCommand, public MapLayersCommand
{
public:
    RemoveTileCommand(QWeakPointer<Names> tiles_order, QWeakPointer<Tileset> tileset, QWeakPointer<MapLayer> map_layers, const TileReference &id):
        QUndoCommand(), TilesOrderCommand(tiles_order), TilesetCommand(tileset), MapLayersCommand(map_layers),
        index{0}, tile_reference{id}, image{}
    {
        index = lockTilesOrder()->indexOf(tile_reference);
        image = lockTileset()->value(tile_reference);

        auto layers = *lockMapLayers();
        for (int j = 0; j < layers.length(); ++j)
            for (int i = 0; i < layers.at(j).length(); ++i)
                if (layers.at(j).at(i) == id)
                    affected_tiles.insert({i, j}, layers.at(j).at(i));
    }

    void undo() final override
    {
        lockTilesOrder()->insert(index, tile_reference);
        lockTileset()->insert(tile_reference, image);

        for (auto &[i, j]: affected_tiles.keys())
            (*lockMapLayers())[j][i] = affected_tiles[{i, j}];
    }

    void redo() final override
    {
        lockTilesOrder()->remove(index);
        lockTileset()->remove(tile_reference);

        for (auto &[i, j]: affected_tiles.keys())
            (*lockMapLayers())[j][i] = {};
    }

private:
    int index;
    TileReference tile_reference;
    QImage image;
    QHash<QPoint, TileReference> affected_tiles;
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
    const double n_tiles = tiles_order? tiles_order->length() : 0;

//  empty tile => +1
    grid_aspect = {
        n_columns,
        qCeil((n_tiles + 1) / n_columns)
    };

    EditorWidget::resize();
}

void TilesetViewWidget::addTiles(const QVector<QImage> &images, const bool undoable)
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!tiles_order.isNull());
    Q_ASSERT(!tileset.isNull());

    if (undoable)
    {
        undo_stack->beginMacro("Add tiles");
        for (auto &pixels: images)
            undo_stack->push(new AddTileCommand(tiles_order, tileset, pixels));
        undo_stack->endMacro();
    }
    else
    {
        for (auto &pixels: images)
        {
            const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            tiles_order->push_back(id);
            tileset->insert(id, pixels);
        }
    }

    update();
}

void TilesetViewWidget::removeTiles(const QVector<TileReference> &tiles)
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!tiles_order.isNull());
    Q_ASSERT(!tileset.isNull());
    Q_ASSERT(!map_layers.isNull());

    undo_stack->beginMacro("Remove tiles");
    for (auto &id: tiles)
        undo_stack->push(new RemoveTileCommand(tiles_order, tileset, map_layers, id));
    undo_stack->endMacro();

    emit tilesRemoved();
}

std::optional<QPoint> TilesetViewWidget::toIJ(const int idx) const
{
    Q_ASSERT(!tiles_order.isNull());
    const int n = tiles_order->length();

    if (idx < 0)
        return QPoint(0, 0);
    else if (idx < n)
        return QPoint((idx+1) % n_columns, (idx+1) / n_columns);
    else
        return {};
}

std::optional<int> TilesetViewWidget::toIndex(const QPoint &ij) const
{
    Q_ASSERT(!tiles_order.isNull());
    const int n = tiles_order->length();
    const int idx = ij.x() + ij.y() * n_columns;

//  idx still takes into account the empty tile
    if (idx < 0 || idx >= n+1)
        return {};
    else
        return idx - 1;
}

//  QPoint's division rounds ; we DON'T want that
static inline QPoint divide(const QPoint &p, const double a)
{
    return {int(p.x() / a), int(p.y() / a)};
}

void TilesetViewWidget::paintTileset(QPainter &painter)
{
    Q_ASSERT(!tiles_order.isNull());
    Q_ASSERT(!tileset.isNull());

    const int n = tiles_order->length();

    Names displayed = *tiles_order;
    if (click_origin || right_click_origin)
    {
        const auto p = right_click_origin? *right_click_origin : *click_origin;

        const auto origin = toIndex(divide(p, tilesize));
        const auto target = toIndex(divide(mouse_cursor, tilesize));

        if (origin && *origin != -1 && target && *target != -1)
        {
            if (drag_mode == MOVE_MODE && click_origin)
                displayed.insert(*target, displayed.takeAt(*origin));
            else if (drag_mode == SWAP_MODE && click_origin)
                displayed.swapItemsAt(*origin, *target);
            else if (drag_mode == SELECTION_MODE && right_click_origin)
                displayed.insert(*target, displayed.takeAt(*origin));
        }
    }

//  toIJ will take care of the empty tile shift
    for (int i = 0; i < n; ++i)
    {
        const QString id = displayed[i];
        const auto p = toIJ(i);
        painter.drawImage(*p * tilesize, tileset->value(id));
    }
}

void TilesetViewWidget::paintSelectionCursors(QPainter &painter)
{
    Q_ASSERT(!selected_tiles.isNull());
    Q_ASSERT(!tiles_order.isNull());

//  more readable and concise, with hardly any performances drop
    QSet<QString> unique_ids;
    for (auto &row: *selected_tiles)
        for (auto &id: row)
            unique_ids.insert(id);

    for (auto &id: unique_ids)
    {
        if (const auto p = toIJ(tiles_order->indexOf(id)))
        {
            const QRect outer = {*p * tilesize, QSize(tilesize-1, tilesize-1)};
            const QColor white64 = {255, 255, 255, 64};
            painter.fillRect(outer, white64);
            painter.setPen(Qt::black);
            painter.drawRect(outer);
            const QRect inner = {*p * tilesize + QPoint(1, 1), QSize(tilesize-3, tilesize-3)};
            painter.setPen(Qt::white);
            painter.drawRect(inner);
        }
    }
}

void TilesetViewWidget::paintCursor(QPainter &painter)
{
    Q_ASSERT(!tiles_order.isNull());
    const QPoint p = divide(mouse_cursor, tilesize);

//  TODO: find nice way to factorise using toIndex()
    const int n = tiles_order->length();
    const int idx = p.x() + p.y() * n_columns;

    const QColor white32 = {255, 255, 255, 32};
    const QColor red32 = {255, 0, 0, 32};
    const QRect draw_rect = {p * tilesize, QSize(tilesize, tilesize)};
//  idx is already shifted for the empty tile => +1
    painter.fillRect(draw_rect, (idx < n+1)? white32 : red32);
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
    paintTileset(painter);
    paintGrid(painter);
    paintSelectionCursors(painter);
    paintCursor(painter);
    if (drag_mode == SELECTION_MODE)
        paintSelectionRect(painter);
}

void TilesetViewWidget::handleTilesSelected()
{
    Q_ASSERT(!tiles_order.isNull());
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
            if (const auto idx = toIndex({i, j}))
                selected_tiles->back().push_back((idx < 0)? "" : tiles_order->at(*idx));
    }
}

void TilesetViewWidget::handleTileModifications()
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!tiles_order.isNull());

    const QPoint p = right_click_origin? *right_click_origin : *click_origin;

    const auto origin = toIndex(divide(p, tilesize));
    const auto target = toIndex(divide(mouse_cursor, tilesize));

    if (origin && *origin != -1 && target && *target != -1)
    {
        if (drag_mode == MOVE_MODE && click_origin)
            undo_stack->push(new MoveTileCommand(tiles_order, *origin, *target));
        else if (drag_mode == SWAP_MODE && click_origin)
            undo_stack->push(new SwapTilesCommand(tiles_order, *origin, *target));
        else if (drag_mode == SELECTION_MODE && right_click_origin)
            undo_stack->push(new MoveTileCommand(tiles_order, *origin, *target));
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
