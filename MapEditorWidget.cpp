#include "MapEditorWidget.hpp"

#include <QPainter>
#include <QMouseEvent>

class MapLayersCommand
{
public:
    explicit MapLayersCommand(QWeakPointer<MapLayer> ptr):
        map_layers_ptr{ptr}
    {}

    QSharedPointer<MapLayer> lockMapLayers()
    {
        auto ptr = map_layers_ptr.toStrongRef();
        Q_ASSERT(!ptr.isNull());

        return ptr;
    }

private:
    QWeakPointer<MapLayer> map_layers_ptr;
};

class SetTilesCommand final: public QUndoCommand, public MapLayersCommand
{
public:
    using Changes = QHash<QPoint, TileReference>;

    SetTilesCommand(QWeakPointer<MapLayer> map_layers, const Changes prev, const Changes next):
        QUndoCommand(), MapLayersCommand(map_layers), prev{prev}, next{next}
    {}

    void undo() final override
    {
        for (auto &[i, j]: prev.keys())
            (*lockMapLayers())[j][i] = prev.value({i, j});
    }

    void redo() final override
    {
        for (auto &[i, j]: next.keys())
            (*lockMapLayers())[j][i] = next.value({i, j});
    }

private:
    Changes prev, next;
};

class SwapLayersCommand final: public QUndoCommand, public MapLayersCommand
{
public:
    SwapLayersCommand(QWeakPointer<MapLayer> map_layers, const MapLayer &prev, const MapLayer &next):
        QUndoCommand(), MapLayersCommand(map_layers), prev{prev}, next{next}
    {}

    void undo() final override { *lockMapLayers() = prev; }
    void redo() final override { *lockMapLayers() = next; }

private:
    MapLayer prev, next;
};

MapEditorWidget::MapEditorWidget(QWidget *parent):
    EditorWidget(parent),
    mouse_cursor{}, click_origin{}, right_click_origin{}
{
    resize();
}

void MapEditorWidget::resize()
{
    if (map_layers)
    {
        const int h = map_layers->length();
        const int w = map_layers->at(0).length();

        grid_aspect = {w, h};
        EditorWidget::resize();
    }
}

void MapEditorWidget::paintTileRects(QPainter &painter)
{
    if (!(selected_tiles && tileset))
        return;

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
    QPainter painter(this);

    paintBackground(painter);

    painter.scale(zoom, zoom);
    {
        painter.drawImage(0, 0, getPaintedLayer());
        if (click_origin || right_click_origin)
            paintTileRects(painter);
    }
    painter.resetTransform();

    paintGrid(painter);
    if (click_origin || right_click_origin)
        paintRectOutlines(painter);
}

void MapEditorWidget::resizeMap(const QSize &size)
{
    MapLayer prev = *map_layers;

    MapLayer next = prev;
    next.resize(size.height(), {});
    for (auto &row: next)
        row.resize(size.width(), {});

    undo_stack->push(new SwapLayersCommand(map_layers, prev, next));
    emit mapResized();
}

void MapEditorWidget::handleTileSetting()
{
    const QRect selection = asLocalRect(*click_origin, mouse_cursor);
    const auto &[x, y] = selection.topLeft();
    const auto &[w, h] = selection.size();

    const int sh = selected_tiles->length();
    const int sw = selected_tiles->at(0).length();

    SetTilesCommand::Changes prev, next;
    for (int j = 0; j < h; ++j)
    {
        for (int i = 0; i < w; ++i)
        {
            const auto prev_id = map_layers->at(y+j).at(x+i);
            const auto next_id = selected_tiles->at(j % sh).at(i % sw);

            if (prev_id != next_id)
            {
                prev[{x+i, y+j}] = prev_id;
                next[{x+i, y+j}] = next_id;
            }
        }
    }
    if (!next.isEmpty())
        undo_stack->push(new SetTilesCommand(map_layers, prev, next));

    emit tilesSet();
}

void MapEditorWidget::handleTileSelection()
{
    const QRect selection = asLocalRect(*right_click_origin, mouse_cursor);
    const auto &[x1, y1] = selection.topLeft();
    const auto &[x2, y2] = selection.bottomRight();

    selected_tiles->clear();
    for (int j = y1; j <= y2; ++j)
    {
        selected_tiles->push_back({});

        for (int i = x1; i <= x2; ++i)
            selected_tiles->back().push_back(map_layers->at(j).at(i));
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
