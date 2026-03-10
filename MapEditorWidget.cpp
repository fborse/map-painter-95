#include "MapEditorWidget.hpp"

#include <QPainter>
#include <QMouseEvent>

class MapLayersCommand
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

    explicit MapLayersCommand(QWeakPointer<MapLayers> ptr):
        map_layers_ptr{ptr}
    {}

    QSharedPointer<MapLayers> lockMapLayers()
    {
        auto ptr = map_layers_ptr.toStrongRef();
        Q_ASSERT(!ptr.isNull());

        return ptr;
    }

private:
    QWeakPointer<MapLayers> map_layers_ptr;
};

static inline uint qHash(const MapLayersCommand::Coordinates &coords, const uint seed = 0)
{
    return seed ^ (
        qHash(coords.i, seed) * 31
      + qHash(coords.j, seed) * 37
      + qHash(coords.k, seed) * 41
    );
}

class SetTilesCommand final: public QUndoCommand, public MapLayersCommand
{
public:
    using Changes = QHash<Coordinates, TileReference>;

    SetTilesCommand(QWeakPointer<MapLayers> map_layers, const Changes prev, const Changes next):
        QUndoCommand(), MapLayersCommand(map_layers), prev{prev}, next{next}
    {}

    void undo() final override
    {
        for (auto &[i, j, k]: prev.keys())
            (*lockMapLayers())[k][j][i] = prev.value({i, j, k});
    }

    void redo() final override
    {
        for (auto &[i, j, k]: next.keys())
            (*lockMapLayers())[k][j][i] = next.value({i, j, k});
    }

private:
    Changes prev, next;
};

class SwapLayersCommand final: public QUndoCommand, public MapLayersCommand
{
public:
    SwapLayersCommand(QWeakPointer<MapLayers> map_layers, const MapLayers &prev, const MapLayers &next):
        QUndoCommand(), MapLayersCommand(map_layers), prev{prev}, next{next}
    {}

    void undo() final override { *lockMapLayers() = prev; }
    void redo() final override { *lockMapLayers() = next; }

private:
    MapLayers prev, next;
};

class InsertLayerCommand final: public QUndoCommand, public MapLayersCommand
{
public:
    InsertLayerCommand(QWeakPointer<MapLayers> map_layers, const int index):
        QUndoCommand(), MapLayersCommand(map_layers), index{index}
    {
        const auto &layers = *lockMapLayers();
        Q_ASSERT(!layers.isEmpty());
        Q_ASSERT(!layers.at(0).isEmpty());

        h = layers.at(0).length();
        w = layers.at(0).at(0).length();
    }

    void undo() final override { lockMapLayers()->remove(index); }

    void redo() final override
    {
        lockMapLayers()->insert(index, MapLayer(h));
        for (auto &row: (*lockMapLayers())[index])
            row.resize(w, {});
    }

private:
    int index;
    int w, h;
};

class RemoveLayerCommand final: public QUndoCommand, public MapLayersCommand
{
public:
    RemoveLayerCommand(QWeakPointer<MapLayers> map_layers, const int index):
        QUndoCommand(), MapLayersCommand(map_layers), index{index}
    {
        Q_ASSERT(index < lockMapLayers()->length());
        removed = lockMapLayers()->at(index);
    }

    void undo() final override { lockMapLayers()->insert(index, removed); }
    void redo() final override { lockMapLayers()->remove(index); }

private:
    int index;
    MapLayer removed;
};

MapEditorWidget::MapEditorWidget(QWidget *parent):
    EditorWidget(parent),
    show_above_layers{true},
    mouse_cursor{}, click_origin{}, right_click_origin{}
{
    resize();
}

void MapEditorWidget::resize()
{
//  here map_layers null is a legit case
    if (map_layers)
    {
        Q_ASSERT(map_layers->length() > 0);
        const int h = map_layers->at(0).length();
        Q_ASSERT(map_layers->at(0).length() > 0);
        const int w = map_layers->at(0).at(0).length();

        grid_aspect = {w, h};
        EditorWidget::resize();
    }
}

void MapEditorWidget::resizeMap(const QSize &size)
{
    Q_ASSERT(!map_layers.isNull());
    MapLayers prev = *map_layers;

    MapLayers next = prev;
    for (auto &layer: next)
    {
        layer.resize(size.height(), {});

        for (auto &row: layer)
            row.resize(size.width(), {});
    }

    undo_stack->push(new SwapLayersCommand(map_layers, prev, next));
    emit mapResized();
}

void MapEditorWidget::insertLayer(const int index)
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!map_layers.isNull());
//  The +1 is because we may be adding at the end
    Q_ASSERT(0 <= index && index < map_layers->length() + 1);

    undo_stack->push(new InsertLayerCommand(map_layers, index));
}

void MapEditorWidget::removeLayer(const int index)
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(!map_layers.isNull());
    Q_ASSERT(0 <= index && index < map_layers->length());

    undo_stack->push(new RemoveLayerCommand(map_layers, index));
}

void MapEditorWidget::paintSimpleTile(QPainter &painter, const QString &id, const QPoint &xy, const QPoint &ij)
{
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(simple_tiles->contains(id));

    const auto &frames = (*simple_tiles)[id].frames;
    const int n = frames.length();
    const int index = qMin(current_frame, n - 1);

    painter.drawImage(xy + ij * tilesize, frames[index]);
}

void MapEditorWidget::paintAutoTile(QPainter &painter, const TileReference &ref, const QPoint &xy, const QPoint &ij)
{
    Q_ASSERT(!autotiles.isNull());
    Q_ASSERT(autotiles->contains(ref.name));

    const auto &frames = (*autotiles)[ref.name].frames;
    const int n = frames.length();
    const int index = qMin(current_frame, n - 1);

    painter.drawImage(xy + ij * tilesize, frames[index].genTile(ref.orientation));
}

void MapEditorWidget::paintTileRects(QPainter &painter)
{
    Q_ASSERT(!selected_tiles.isNull());
    Q_ASSERT(!simple_tiles.isNull());

    const auto click = click_origin? *click_origin : *right_click_origin;
    const QRect selection = asLocalRect(click, mouse_cursor);

    const auto &[x, y] = selection.topLeft() * tilesize;
    const auto &[w, h] = selection.size();

    if (click_origin && !selected_tiles->isEmpty())
    {
        const int sh = selected_tiles->length();
        const int sw = selected_tiles->at(0).length();

        for (int j = 0; j < h; ++j)
        {
            for (int i = 0; i < w; ++i)
            {
                if (const auto ref = selected_tiles->at(j % sh).at(i % sw))
                {
                    if (ref.autotile)
                        paintAutoTile(painter, ref, {x, y}, {i, j});
                    else
                        paintSimpleTile(painter, ref.name, {x, y}, {i, j});
                }
            }
        }
    }
    else if (right_click_origin)
    {
        const QColor white64 = {255, 255, 255, 64};
        painter.fillRect(QRect(QPoint(x, y), QSize(w, h) * tilesize), white64);
    }
}

void MapEditorWidget::paintRectOutlines(QPainter &painter)
{
    const int unit = tilesize * zoom;

    const QPoint click = click_origin? *click_origin : *right_click_origin;
    const QRect selection = asLocalRect(click, mouse_cursor);

    const QPoint top_left = selection.topLeft() * unit;
    const QSize size = selection.size() * unit;

//  drawRect has a weird way of overshooting by one pixel...
    const QRect draw_rect(top_left, size - QSize(1, 1));

    painter.setPen(Qt::white);
    painter.drawRect(draw_rect);
}

void MapEditorWidget::paintEvent(QPaintEvent *)
{
    Q_ASSERT(!map_layers.isNull());
    QPainter painter(this);

    paintBackground(painter);

    painter.scale(zoom, zoom);
    {
        for (int k = 0; k <= current_layer; ++k)
            painter.drawImage(0, 0, getPaintedLayer(k));

        if (show_above_layers)
        {
            painter.setOpacity(0.5);
            for (int k = current_layer+1; k < map_layers->length(); ++k)
                painter.drawImage(0, 0, getPaintedLayer(k));
            painter.setOpacity(1);
        }

        if (click_origin || right_click_origin)
            paintTileRects(painter);
    }
    painter.resetTransform();

    paintGrid(painter);
    if (click_origin || right_click_origin)
        paintRectOutlines(painter);
}

static inline std::optional<QString> name_at(const MapLayers &map_layers, const SetTilesCommand::Changes &next, const SetTilesCommand::Coordinates &p)
{
    if (next.contains(p))
        return next[p].name;
    else if (p.i < 0 || p.j < 0 || p.k < 0)
        return {};
    else if (p.k >= map_layers.length())
        return {};
    else if (p.j >= map_layers[p.k].length())
        return {};
    else if (p.i >= map_layers[p.k][p.j].length())
        return {};
    else
        return map_layers[p.k][p.j][p.i].name;
}

//  Remember modified RPG Maker scheme (see Types.hpp)
//  top-left            top2                top1                top-right
//  left2               middle4             middle3             right2
//  left1               middle2             middle1             right1
//  bottom-left         bottom2             bottom1             bottom-right
//  top-left-joint      top-right-joint     bottom-left-joint   bottom-right-joint

static constexpr int TOP_LEFT = 0;
static constexpr int TOP2 = 1;
static constexpr int TOP1 = 2;
static constexpr int TOP_RIGHT = 3;
static constexpr int LEFT2 = 4;
static constexpr int MIDDLE4 = 5;
static constexpr int MIDDLE3 = 6;
static constexpr int RIGHT2 = 7;
static constexpr int LEFT1 = 8;
static constexpr int MIDDLE2 = 9;
static constexpr int MIDDLE1 = 10;
static constexpr int RIGHT1 = 11;
static constexpr int BOTTOM_LEFT = 12;
static constexpr int BOTTOM2 = 13;
static constexpr int BOTTOM1 = 14;
static constexpr int BOTTOM_RIGHT = 15;
static constexpr int TOP_LEFT_JOINT = 16;
static constexpr int TOP_RIGHT_JOINT = 17;
static constexpr int BOTTOM_LEFT_JOINT = 18;
static constexpr int BOTTOM_RIGHT_JOINT = 19;

struct AutoTileDirections
{
    bool left = false, top = false, right = false, bottom = false;
    bool top_left = false, top_right = false, bottom_left = false, bottom_right = false;

    int getTopLeft() const
    {
        if (left && top)
            return top_left? MIDDLE1 : TOP_LEFT_JOINT;
        else if (left)
            return TOP1;
        else if (top)
            return LEFT1;
        else
            return TOP_LEFT;
    }

    int getTopRight() const
    {
        if (right && top)
            return top_right? MIDDLE2 : TOP_RIGHT_JOINT;
        else if (right)
            return TOP2;
        else if (top)
            return RIGHT1;
        else
            return TOP_RIGHT;
    }

    int getBottomLeft() const
    {
        if (left && bottom)
            return bottom_left? MIDDLE3 : BOTTOM_LEFT_JOINT;
        else if (left)
            return BOTTOM1;
        else if (bottom)
            return LEFT2;
        else
            return BOTTOM_LEFT;
    }

    int getBottomRight() const
    {
        if (right && bottom)
            return bottom_right? MIDDLE4 : BOTTOM_RIGHT_JOINT;
        else if (right)
            return BOTTOM2;
        else if (bottom)
            return RIGHT2;
        else
            return BOTTOM_RIGHT;
    }

    Orientation toOrientation() const
    {
        Orientation orientation;

        orientation.top_left = getTopLeft();
        orientation.top_right = getTopRight();
        orientation.bottom_left = getBottomLeft();
        orientation.bottom_right = getBottomRight();

        return orientation;
    }

    static AutoTileDirections fromContextAt(const MapLayers &map_layers, const SetTilesCommand::Changes &next, const QString id, const SetTilesCommand::Coordinates &ijk)
    {
        AutoTileDirections directions;

        const auto &[i, j, k] = ijk;
        if (const auto name = name_at(map_layers, next, {i-1, j, k}))
            directions.left = (*name == id);
        if (const auto name = name_at(map_layers, next, {i-1, j-1, k}))
            directions.top_left = (*name == id);
        if (const auto name = name_at(map_layers, next, {i, j-1, k}))
            directions.top = (*name == id);
        if (const auto name = name_at(map_layers, next, {i+1, j-1, k}))
            directions.top_right = (*name == id);
        if (const auto name = name_at(map_layers, next, {i+1, j, k}))
            directions.right = (*name == id);
        if (const auto name = name_at(map_layers, next, {i+1, j+1, k}))
            directions.bottom_right = (*name == id);
        if (const auto name = name_at(map_layers, next, {i, j+1, k}))
            directions.bottom = (*name == id);
        if (const auto name = name_at(map_layers, next, {i-1, j+1, k}))
            directions.bottom_left = (*name == id);

        return directions;
    }
};

static inline void reorient_prev(SetTilesCommand::Changes &prev, SetTilesCommand::Changes &next, const QVector<SetTilesCommand::Coordinates> &original_coords, const MapLayers &map_layers)
{}

static inline void reorient_next(SetTilesCommand::Changes &prev, SetTilesCommand::Changes &next, const QVector<SetTilesCommand::Coordinates> &original_coords, const MapLayers &map_layers)
{
    QHash<SetTilesCommand::Coordinates, AutoTileDirections> affected_neighbours;

    for (auto &[i, j, k]: original_coords)
    {
        auto &ref = next[{i, j, k}];

        if (ref.autotile)
        {
            const auto directions =
                AutoTileDirections::fromContextAt(map_layers, next, ref.name, {i, j, k});
            ref.orientation = directions.toOrientation();

        //  these coordinates are guaranteed valid when the boolean is true
        //  additionally, the direction values are preset to false
        //  also, "constant-time" retrieval, as opposed to original_coords.contains()
            if (directions.left && !next.contains({i-1, j, k}))
                affected_neighbours[{i-1, j, k}].right = true;
            if (directions.top_left && !next.contains({i-1, j-1, k}))
                affected_neighbours[{i-1, j-1, k}].bottom_right = true;
            if (directions.top && !next.contains({i, j-1, k}))
                affected_neighbours[{i, j-1, k}].bottom = true;
            if (directions.top_right && !next.contains({i+1, j-1, k}))
                affected_neighbours[{i+1, j-1, k}].bottom_left = true;
            if (directions.right && !next.contains({i+1, j, k}))
                affected_neighbours[{i+1, j, k}].left = true;
            if (directions.bottom_right && !next.contains({i+1, j+1, k}))
                affected_neighbours[{i+1, j+1, k}].top_left = true;
            if (directions.bottom && !next.contains({i, j+1, k}))
                affected_neighbours[{i, j+1, k}].top = true;
            if (directions.bottom_left && !next.contains({i-1, j+1, k}))
                affected_neighbours[{i-1, j+1, k}].top_right = true;
        }
    }

    for (auto &[i, j, k]: affected_neighbours.keys())
    {
        auto ref = map_layers[k][j][i];
        prev[{i, j, k}] = ref;

        const auto directions =
            AutoTileDirections::fromContextAt(map_layers, next, ref.name, {i, j, k});
        ref.orientation = directions.toOrientation();
        next[{i, j, k}] = ref;
    }
}

void MapEditorWidget::handleTileSetting()
{
    Q_ASSERT(!selected_tiles.isNull());
    Q_ASSERT(!map_layers.isNull());
    Q_ASSERT(click_origin.has_value());
    Q_ASSERT(current_layer < map_layers->length());

    const QRect selection = asLocalRect(*click_origin, mouse_cursor);
    const auto &[x, y] = selection.topLeft();
    const auto &[w, h] = selection.size();

    const int sh = selected_tiles->length();
    const int sw = selected_tiles->at(0).length();

    const MapLayer &layer = map_layers->at(current_layer);

    SetTilesCommand::Changes prev, next;
    for (int j = 0; j < h; ++j)
    {
        for (int i = 0; i < w; ++i)
        {
            const auto prev_ref = layer.at(y+j).at(x+i);
            const auto next_ref = selected_tiles->at(j % sh).at(i % sw);

            if (prev_ref != next_ref)
            {
                prev[{x+i, y+j, current_layer}] = prev_ref;
                next[{x+i, y+j, current_layer}] = next_ref;
            }
        }
    }
    if (!next.isEmpty())
    {
        const auto original_coords = next.keys();
        reorient_prev(prev, next, original_coords, *map_layers);
        reorient_next(prev, next, original_coords, *map_layers);
        undo_stack->push(new SetTilesCommand(map_layers, prev, next));
    }

    emit tilesSet();
}

void MapEditorWidget::handleTileSelection()
{
    Q_ASSERT(!selected_tiles.isNull());
    Q_ASSERT(!map_layers.isNull());
    Q_ASSERT(current_layer < map_layers->length());
    Q_ASSERT(right_click_origin.has_value());

    const QRect selection = asLocalRect(*right_click_origin, mouse_cursor);
    const auto &[x1, y1] = selection.topLeft();
    const auto &[x2, y2] = selection.bottomRight();

    const auto &layer = map_layers->at(current_layer);
    selected_tiles->clear();
    for (int j = y1; j <= y2; ++j)
    {
        selected_tiles->push_back({});

        for (int i = x1; i <= x2; ++i)
            selected_tiles->back().push_back(layer.at(j).at(i));
    }

    emit tileSelected();
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

void MapEditorWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouse_cursor = clamp(getWidgetRect(), event->pos());

    update();
}

void MapEditorWidget::mousePressEvent(QMouseEvent *event)
{
    if (getWidgetRect().contains(event->pos()))
    {
        if (event->button() == Qt::LeftButton)
            click_origin = event->pos();
        if (event->button() == Qt::RightButton)
            right_click_origin = event->pos();
    }

    update();
}

void MapEditorWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (selected_tiles && !selected_tiles->isEmpty())
            handleTileSetting();
        click_origin = {};
    }

    if (event->button() == Qt::RightButton)
    {
        handleTileSelection();
        right_click_origin = {};
    }

    update();
}
