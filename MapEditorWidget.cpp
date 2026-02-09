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

MapEditorWidget::MapEditorWidget(QWidget *parent):
    EditorWidget(parent),
    mouse_cursor{}, left_click{}, right_click{}
{
    resize();
}

void MapEditorWidget::paintLayers(QPainter &painter)
{
    if (!(tileset && map_layers))
        return;

    const int unit = tilesize * zoom;

    for (int j = 0; j < map_layers->length(); ++j)
    {
        for (int i = 0; i < map_layers->at(j).length(); ++i)
        {
            const auto id = map_layers->at(j).at(i);

            if (tileset->contains(id))
                painter.drawImage(i * unit, j * unit, tileset->value(id));
        }
    }
}

void MapEditorWidget::paintSelectionRect(QPainter &painter)
{
    const int unit = tilesize * zoom;

    const QRect selection = asLocalRect(*right_click, mouse_cursor);
    const QPoint top_left = selection.topLeft() * unit;
    const QSize size = selection.size() * unit;
//  drawRect has a weird way of overshooting by one pixel...
    const QRect draw_rect(top_left, size - QSize(1, 1));

    painter.setPen(Qt::white);
    painter.drawRect(draw_rect);
    const QColor white64 = {255, 255, 255, 64};
    painter.fillRect(draw_rect, white64);
}

void MapEditorWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    paintBackground(painter);
    paintLayers(painter);
    paintGrid(painter);
    if (right_click)
        paintSelectionRect(painter);
}

void MapEditorWidget::handleTileSelection()
{
    const QRect selection = asLocalRect(*right_click, mouse_cursor);
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
            left_click = event->pos();
        if (event->button() == Qt::RightButton)
            right_click = event->pos();
    }

    update();
}

void MapEditorWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        left_click = {};

    if (event->button() == Qt::RightButton)
    {
        handleTileSelection();
        right_click = {};
    }

    update();
}
