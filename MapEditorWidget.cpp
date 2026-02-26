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

void MapEditorWidget::paintTileRects(QPainter &painter)
{
    Q_ASSERT(!selected_tiles.isNull());
    Q_ASSERT(!tileset.isNull());

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
                const auto id = selected_tiles->at(j % sh).at(i % sw);

                if (tileset->contains(id))
                    painter.drawImage(x + i * tilesize, y + j * tilesize, tileset->value(id));
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
            const auto prev_id = layer.at(y+j).at(x+i);
            const auto next_id = selected_tiles->at(j % sh).at(i % sw);

            if (prev_id != next_id)
            {
                prev[{x+i, y+j, current_layer}] = prev_id;
                next[{x+i, y+j, current_layer}] = next_id;
            }
        }
    }
    if (!next.isEmpty())
        undo_stack->push(new SetTilesCommand(map_layers, prev, next));

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
