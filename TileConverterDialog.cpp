#include "TileConverterDialog.hpp"
#include "ui_TileConverterDialog.h"

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

template <typename T>
class TilesCommand
{
public:
    TilesCommand(QWeakPointer<T> tiles): tiles_ptr{tiles} {}
    QSharedPointer<T> lockTiles() { return lock_ptr(tiles_ptr); }

private:
    QWeakPointer<T> tiles_ptr;
};

template <typename T>
class AddTileCommand: public QUndoCommand, public TilesOrderCommand, public TilesCommand<Tileset<T>>
{
public:
    AddTileCommand(QWeakPointer<Names> tiles_order, QWeakPointer<Tileset<T>> tiles, const QString &id, const T &tile):
        QUndoCommand(), TilesOrderCommand(tiles_order), TilesCommand<Tileset<T>>(tiles),
    //  see Types.hpp for why {0, 3, 12, 15} (single isolated tile)
        index{0}, added_tile{tile}, added_ref{id, false, {0, 3, 12, 15}}
    {
        index = lockTilesOrder()->length();
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

class AddAutoTileCommand final: public AddTileCommand<AutoTile>
{
public:
    AddAutoTileCommand(QWeakPointer<Names> tiles_order, QWeakPointer<AutoTiles> tiles, const QString &id, const AutoTile &tile):
        AddTileCommand(tiles_order, tiles, id, tile)
    {
        added_ref.autotile = true;
    }
};

SelectionGridWidget::SelectionGridWidget(QWidget *parent):
    QWidget(parent), grid_aspect{0, 0}, tilesize{0}, mouse_position{}
{}

QPoint SelectionGridWidget::toIJ(const int index) const
{
    const int w = grid_aspect.width();

    return {index % w, index / w};
}

int SelectionGridWidget::toIndex(const QPoint &grid_coordinates) const
{
    const auto &[i, j] = grid_coordinates;
    const int w = grid_aspect.width();

    return i + j * w;
}

void SelectionGridWidget::paintBackground(QPainter &painter)
{
    const auto &[w, h] = grid_aspect;
    const int s = tilesize;

    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};

    for (int j = 0; j < h; ++j)
    {
        for (int i = 0; i < w; ++i)
        {
            painter.fillRect(i * s, j * s, s/2, s/2, dark);
            painter.fillRect(i * s + s/2, j * s, s/2, s/2, light);
            painter.fillRect(i * s, j * s + s/2, s/2, s/2, light);
            painter.fillRect(i * s + s/2, j * s + s/2, s/2, s/2, dark);
        }
    }
}

void SelectionGridWidget::paintGrid(QPainter &painter)
{
    const QColor white128 = {255, 255, 255, 128};
    const auto &[w, h] = grid_aspect;

    for (int j = 0; j < h; ++j)
        painter.fillRect(0, j * tilesize, w * tilesize, 1, white128);
    for (int i = 0; i < w; ++i)
        painter.fillRect(i * tilesize, 0, 1, h * tilesize, white128);
    for (int j = 1; j < h+1; ++j)
        painter.fillRect(0, j * tilesize - 1, w * tilesize, 1, white128);
    for (int i = 1; i < w+1; ++i)
        painter.fillRect(i * tilesize - 1, 0, 1, h * tilesize, white128);
}

void SelectionGridWidget::paintSelectionRect(QPainter &painter, const QPoint &grid_coordinates)
{
    const auto &[x, y] = grid_coordinates * tilesize;

    painter.setPen(Qt::black);
    painter.drawRect(x, y, tilesize, tilesize);
    painter.setPen(Qt::white);
    painter.drawRect(x+1, y+1, tilesize-2, tilesize-2);
}

static inline QPoint divide(const QPoint &p, const double f)
{
    return {int(p.x() / f), int(p.y() / f)};
}

void SelectionGridWidget::paintHoverCursor(QPainter &painter, const bool valid_coordinates)
{
    const QColor white64 = {255, 255, 255, 64};
    const QColor red64 = {255, 0, 0, 64};

    const QPoint p = divide(mouse_position, tilesize);
    const QRect r = {p * tilesize, QSize(1, 1) * tilesize};

    painter.fillRect(r, valid_coordinates? white64 : red64);
}

void SelectionGridWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouse_position = event->pos();

    update();
}

TileSelectionWidget::TileSelectionWidget(QWidget *parent):
    SelectionGridWidget(parent),
    captions{}, n_columns{8}, selected{-1}
{}

void TileSelectionWidget::addCaption(const QString &id, const QImage &caption)
{
    captions.push_back({id, caption});
}

void TileSelectionWidget::resize()
{
    const int n = captions.length();

    const int w = qMin(n, n_columns);
    const int h = qCeil(n / double(n_columns));

    setGridAspect(w, h);
    SelectionGridWidget::resize();
}

void TileSelectionWidget::paintCaptions(QPainter &painter)
{
    for (int i = 0; i < captions.length(); ++i)
    {
        const QPoint p = {i % n_columns, i / n_columns};
        painter.drawImage(p * tilesize, captions[i].second);
    }
}

void TileSelectionWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    paintBackground(painter);
    paintCaptions(painter);
    paintGrid(painter);

    const QPoint p = divide(mouse_position, tilesize);
    const int n = captions.length();
    paintHoverCursor(painter, (toIndex(p) < n));

    if (selected >= 0)
        paintSelectionRect(painter, toIJ(selected));
}

void TileSelectionWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        const int index = toIndex(divide(mouse_position, tilesize));

        if (index < captions.length())
        {
            emit tileSelected(captions[index].first);

            selected = index;
            update();
        }
    }
}

SimpleTileViewWidget::SimpleTileViewWidget(QWidget *parent):
    QWidget(parent),
    metatilesize{0}, tile{},
    metatile_coordinates{0, 0}, mouse_position{0, 0}, move_offset{}
{
    setFixedSize(0, 0);
}

void SimpleTileViewWidget::setTileCaption(const QImage &simple_tile)
{
    tile = simple_tile;
    setFixedSize(tile.size());
    update();
}

QRect SimpleTileViewWidget::getMetaTileRect() const
{
    Q_ASSERT(metatilesize > 0);
    const QSize s = {metatilesize, metatilesize};

    return QRect(metatile_coordinates, s);
}

QImage SimpleTileViewWidget::getMetatile() const
{
    Q_ASSERT(!tile.isNull());
    return tile.copy(getMetaTileRect());
}

void SimpleTileViewWidget::paintBackground(QPainter &painter)
{
    const int s = tile.width();
    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};

    painter.fillRect(0, 0, s, s, dark);
    painter.fillRect(s, 0, s, s, light);
    painter.fillRect(0, s, s, s, light);
    painter.fillRect(s, s, s, s, dark);
}

void SimpleTileViewWidget::paintMetatileCursor(QPainter &painter)
{
    const auto &[x, y] = metatile_coordinates;
    const int s = metatilesize;

    painter.setPen(Qt::black);
    painter.drawRect(x, y, s, s);

    painter.setPen(Qt::white);
    painter.drawRect(x+1, y+1, s-2, s-2);

    const QColor white64 = {255, 255, 255, 64};
    const QColor white128 = {255, 255, 255, 128};

    const QRect r(x, y, s, s);
    painter.fillRect(r, r.contains(mouse_position)? white128 : white64);
}

void SimpleTileViewWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    paintBackground(painter);
    painter.drawImage(0, 0, tile);
    paintMetatileCursor(painter);
}

static inline void clamp_to(QPoint &p, const QRect &r)
{
    if (p.x() < r.left())
        p.setX(r.left());
    if (p.x() > r.right())
        p.setX(r.right());

    if (p.y() < r.top())
        p.setY(r.top());
    if (p.y() > r.bottom())
        p.setY(r.bottom());
}

void SimpleTileViewWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouse_position = event->pos();

    if (move_offset)
    {
        metatile_coordinates = mouse_position - *move_offset;

        const QSize s = {metatilesize, metatilesize};
        clamp_to(metatile_coordinates, QRect({0, 0}, size() - s));
    }

    update();
}

void SimpleTileViewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        const auto &[x, y] = metatile_coordinates;
        const int s = metatilesize;

        if (QRect(x, y, s, s).contains(mouse_position))
            move_offset = mouse_position - metatile_coordinates;
    }
}

void SimpleTileViewWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        move_offset = {};
}

GeneratedTilesWidget::GeneratedTilesWidget(QWidget *parent):
    SelectionGridWidget(parent), captions{}, new_caption{}, selected{-1}
{
    setGridAspect(1, 1);
}

void GeneratedTilesWidget::addCaption(const QString &id, const QImage &caption)
{
    captions.push_back({id, caption});
    setGridAspect(captions.length() + 1, 1);
    resize();
}

template <typename T>
static inline int index_of(const QVector<QPair<QString, T>> &xs, const QString &id)
{
    for (int i = 0; i < xs.length(); ++i)
        if (xs[i].first == id)
            return i;

    return -1;
}

void GeneratedTilesWidget::removeCaption(const QString &id)
{
    const int index = index_of(captions, id);
    Q_ASSERT(index >= 0);

    captions.remove(index);
    setGridAspect(captions.length() + 1, 1);
    resize();
}

void GeneratedTilesWidget::paintCaptions(QPainter &painter)
{
    for (int i = 0; i < captions.length(); ++i)
        painter.drawImage(i * tilesize, 0, captions[i].second);
}

void GeneratedTilesWidget::paintNewTileCaption(QPainter &painter)
{
    const int n = captions.length();
    painter.drawImage(n * tilesize, 0, new_caption);
}

void GeneratedTilesWidget::paintAddSymbol(QPainter &painter)
{
    const int x = captions.length() * tilesize;
    const int y = 0;

//  horizontal
    painter.fillRect(x + tilesize/4, y + tilesize/2 - 1, tilesize/2, 2, Qt::white);
//  vertical
    painter.fillRect(x + tilesize/2 - 1, y + tilesize/4, 2, tilesize/2, Qt::white);
}

void GeneratedTilesWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    paintBackground(painter);
    paintCaptions(painter);
    paintNewTileCaption(painter);
    paintAddSymbol(painter);
    paintGrid(painter);
//  hovering the plus symbol is also valid coord
    paintHoverCursor(painter, true);

    if (selected >= 0)
        paintSelectionRect(painter, {selected, 0});
}

void GeneratedTilesWidget::mousePressEvent(QMouseEvent *event)
{
    const int index = toIndex(divide(mouse_position, tilesize));

    if (event->button() == Qt::LeftButton)
    {
        if (index < captions.length())
            emit tileSelected(captions[index].first);
        else
            emit addTileClicked();
    }

    if (event->button() == Qt::RightButton)
    {
        if (index < captions.length())
            emit removeTileClicked(captions[index].first);
    }
}

MetatilesViewWidget::MetatilesViewWidget(QWidget *parent):
    SelectionGridWidget(parent), origins(20), metatiles(20)
{
//  RPG Maker scheme
    setGridAspect(4, 6);
}

void MetatilesViewWidget::setMetatileOrigin(const int index, const QString &id, const QRect &rect)
{
    Q_ASSERT(0 <= index && index < origins.length());
    origins[index] = {id, rect};
}

void MetatilesViewWidget::setMetatileCaption(const int index, const QImage &pixels)
{
    Q_ASSERT(0 <= index && index < metatiles.length());
    metatiles[index] = pixels.scaled(tilesize, tilesize);
    update();
}

void MetatilesViewWidget::paintMetatiles(QPainter &painter)
{
//  top-left 2x2 depict a single isolated tile
//  see Types.hpp for why these values
    painter.drawImage(0, 0, metatiles[0]);
    painter.drawImage(tilesize, 0, metatiles[3]);
    painter.drawImage(0, tilesize, metatiles[12]);
    painter.drawImage(tilesize, tilesize, metatiles[15]);

//  top-right 2x2 are the joints
    painter.drawImage(2 * tilesize, 0, metatiles[16]);
    painter.drawImage(3 * tilesize, 0, metatiles[17]);
    painter.drawImage(2 * tilesize, tilesize, metatiles[18]);
    painter.drawImage(3 * tilesize, tilesize, metatiles[19]);

//  now the rest is linear
    const int w = grid_aspect.width();
    for (int i = 0; i < 16; ++i)
        painter.drawImage((i%w) * tilesize, (i/w + 2) * tilesize, metatiles[i]);
}

void MetatilesViewWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    paintBackground(painter);
    paintMetatiles(painter);
    paintGrid(painter);
//  no bad cursor position here
    paintHoverCursor(painter, true);
}

void MetatilesViewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        const auto &[i, j] = divide(mouse_position, tilesize);

        if (j < 2)
        {
            const QHash<QPoint, int> coords = {
            //  top-left 2x2 depict a single isolated tile
                {{0, 0}, 0}, {{1, 0}, 3}, {{0, 1}, 12}, {{1, 1}, 15},
            //  top-right 2x2 are the joint
                {{2, 0}, 16}, {{3, 0}, 17}, {{2, 1}, 18}, {{3, 1}, 19}
            };

            Q_ASSERT(coords.contains({i, j}));
            emit metatileClicked(coords[{i, j}]);
        }
        else
        {
            const int w = grid_aspect.width();
            emit metatileClicked(i + (j-2) * w);
        }
    }
}

//  upper-left-joint    top-left    top1        top2        top-right       upper-right-joint
//        -----         left1       middle1     middle2     right1                -----
//        -----         left2       middle3     middle4     right2                -----
//  bottom-left-joint   bottom-left bottom1     bottom2     bottom-right    bottom-right-joint
const QHash<QPoint, int> MetatileSelectionWidget::metatile_coordinates({
//  top-left quadrant
    {QPoint(0, 0), 16}, {QPoint(1, 0), 0}, {QPoint(2, 0), 2},
                        {QPoint(1, 1), 8}, {QPoint(2, 1), 10},
//  top-right quadrant
    {QPoint(3, 0), 1}, {QPoint(4, 0), 3}, {QPoint(5, 0), 17},
    {QPoint(3, 1), 9}, {QPoint(4, 1), 11},
//  bottom-left quadrant
                        {QPoint(1, 2), 4}, {QPoint(2, 2), 6},
    {QPoint(0, 3), 18}, {QPoint(1, 3), 12}, {QPoint(2, 3), 14},
//  bottom-right quadrant
    {QPoint(3, 2), 5}, {QPoint(4, 2), 7},
    {QPoint(3, 3), 13}, {QPoint(4, 3), 15}, {QPoint(5, 3), 19}
});

MetatileSelectionWidget::MetatileSelectionWidget(QWidget *parent):
    SelectionGridWidget(parent), metatiles(20),
//  see above how these coordinates form a single isolate tile
    top_left(1, 0), top_right(4, 0), bottom_left(1, 3), bottom_right(4, 3),
    dragged{}
{
//  see above scheme for why this grid
    setGridAspect(6, 4);
}

void MetatileSelectionWidget::setMetatile(const int index, const QImage &metatile)
{
    Q_ASSERT(0 <= index && index < metatiles.length());
    metatiles[index] = metatile;
}

Orientation MetatileSelectionWidget::getOrientation() const
{
    return {
        metatile_coordinates[top_left], metatile_coordinates[top_right],
        metatile_coordinates[bottom_left], metatile_coordinates[bottom_right]
    };
}

QImage MetatileSelectionWidget::assembleCaption() const
{
    const int s = tilesize / 2;

    QImage tile(tilesize, tilesize, QImage::Format_ARGB32_Premultiplied);
    tile.fill(Qt::transparent);

    QPainter painter(&tile);
    painter.drawImage(0, 0, metatiles[metatile_coordinates[top_left]].scaled(s, s));
    painter.drawImage(s, 0, metatiles[metatile_coordinates[top_right]].scaled(s, s));
    painter.drawImage(0, s, metatiles[metatile_coordinates[bottom_left]].scaled(s, s));
    painter.drawImage(s, s, metatiles[metatile_coordinates[bottom_right]].scaled(s, s));

    return tile;
}

void MetatileSelectionWidget::paintMetatiles(QPainter &painter)
{
    for (auto &p: metatile_coordinates.keys())
    {
        const QImage &metatile = metatiles[metatile_coordinates[p]];

        if (!metatile.isNull())
            painter.drawImage(p * tilesize, metatile);
    }
}

void MetatileSelectionWidget::paintDividers(QPainter &painter)
{
    const auto &[w, h] = grid_aspect;
    painter.fillRect(0, h/2 * tilesize - 1, w * tilesize, 2, Qt::white);
    painter.fillRect(w/2 * tilesize - 1, 0, 2, h * tilesize, Qt::white);
}

static inline void draw_outline(QPainter &painter, const QPoint &top_left, const int tilesize)
{
    painter.drawRect(QRect(top_left, QSize(tilesize-1, tilesize-1)));
}

static inline void draw_hline(QPainter &painter, const QPoint &left, const int length)
{
    painter.fillRect(left.x(), left.y(), length, 1, Qt::white);
}

static inline void draw_vline(QPainter &painter, const QPoint &top, const int length)
{
    painter.fillRect(top.x(), top.y(), 1, length, Qt::white);
}

void MetatileSelectionWidget::paintSelectionCursors(QPainter &painter)
{
    const int l = 2 * tilesize / 3;

    painter.setPen(Qt::black);
    draw_outline(painter, top_left * tilesize, tilesize);
    draw_hline(painter, top_left * tilesize + QPoint(1, 1), l);
    draw_vline(painter, top_left * tilesize + QPoint(1, 1), l);

    draw_outline(painter, top_right * tilesize, tilesize);
    draw_hline(painter, top_right * tilesize + QPoint(tilesize-1-l, 1), l);
    draw_vline(painter, top_right * tilesize + QPoint(tilesize-2, 1), l);

    draw_outline(painter, bottom_left * tilesize, tilesize);
    draw_hline(painter, bottom_left * tilesize + QPoint(1, tilesize-2), l);
    draw_vline(painter, bottom_left * tilesize + QPoint(1, tilesize-1-l), l);

    draw_outline(painter, bottom_right * tilesize, tilesize);
    draw_hline(painter, bottom_right * tilesize + QPoint(tilesize-1-l, tilesize-2), l);
    draw_vline(painter, bottom_right * tilesize + QPoint(tilesize-2, tilesize-1-l), l);
}

void MetatileSelectionWidget::paintHoverCursor(QPainter &painter)
{
    const auto &[i, j] = divide(mouse_position, tilesize);

    bool valid = true;
    if (i == 0 && (j == 1 || j == 2))
        valid = false;
    if (i == 5 && (j == 1 || j == 2))
        valid = false;

    SelectionGridWidget::paintHoverCursor(painter, valid);
}

void MetatileSelectionWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    paintBackground(painter);
    paintMetatiles(painter);
    paintGrid(painter);
    paintDividers(painter);

    paintSelectionCursors(painter);
    paintHoverCursor(painter);
}

void MetatileSelectionWidget::mouseMoveEvent(QMouseEvent *event)
{
    SelectionGridWidget::mouseMoveEvent(event);

    if (dragged)
    {
        const QPoint p = divide(mouse_position, tilesize);

        switch (*dragged)
        {
        case TOP_LEFT:
            if (QRect(0, 0, 3, 2).contains(p) && p != QPoint(0, 1))
                top_left = p;
            break;
        case TOP_RIGHT:
            if (QRect(3, 0, 3, 2).contains(p) && p != QPoint(5, 1))
                top_right = p;
            break;
        case BOTTOM_LEFT:
            if (QRect(0, 2, 3, 2).contains(p) && p != QPoint(0, 2))
                bottom_left = p;
            break;
        case BOTTOM_RIGHT:
            if (QRect(3, 2, 3, 2).contains(p) && p != QPoint(5, 2))
                bottom_right = p;
            break;
        default:
            throw std::logic_error("Invalid enum value for 'dragged'");
        }

        emit selectedMoved();
    }
}

void MetatileSelectionWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        const QSize s = {tilesize, tilesize};

        if (QRect(top_left * tilesize, s).contains(mouse_position))
            dragged = TOP_LEFT;
        else if (QRect(top_right * tilesize, s).contains(mouse_position))
            dragged = TOP_RIGHT;
        else if (QRect(bottom_left * tilesize, s).contains(mouse_position))
            dragged = BOTTOM_LEFT;
        else if (QRect(bottom_right * tilesize, s).contains(mouse_position))
            dragged = BOTTOM_RIGHT;
    }
}

void MetatileSelectionWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        dragged = {};
}

TileConverterDialog::TileConverterDialog(QWidget *parent):
    QDialog(parent), ui(new Ui::TileConverterDialog),
    tilesize{0},
    undo_stack_ptr{nullptr},
    simple_tiles_order_ptr{nullptr}, autotiles_order_ptr{nullptr},
    simple_tiles_ptr{nullptr}, autotiles_ptr{nullptr},
    selected_simple_tile{}, selected_autotile{},
    added_simple_tiles{}, added_autotiles{}, added_metatile_origins{}
{
    ui->setupUi(this);

    ui->simpleTileViewWidget->setEnabled(false);
    ui->metatilesViewWidget->setEnabled(false);
    ui->metatileSelectionWidget->setEnabled(false);
}

TileConverterDialog::~TileConverterDialog()
{
    delete ui;
}

void TileConverterDialog::onSelectedSimpleTileChanged(const QString id)
{
    selected_simple_tile = id;

    auto simple_tiles = simple_tiles_ptr.toStrongRef();
    Q_ASSERT(!simple_tiles.isNull());

    Q_ASSERT(simple_tiles->contains(id));
    const auto &frames = simple_tiles->value(id).frames;
    Q_ASSERT(!frames.isEmpty());

    ui->simpleTileViewWidget->setTileCaption(frames[0]);
    ui->simpleTileViewWidget->setEnabled(true);

    ui->metatilesViewWidget->setEnabled(true);
}

void TileConverterDialog::onMetaTileClicked(const int index)
{
    const QRect rect = ui->simpleTileViewWidget->getMetaTileRect();
    const QImage pixels = ui->simpleTileViewWidget->getMetatile();
    ui->metatilesViewWidget->setMetatileOrigin(index, selected_simple_tile, rect);
    ui->metatilesViewWidget->setMetatileCaption(index, pixels);
}

static inline int get_n_frames(const SimpleTiles &simple_tiles, const MetatilesViewWidget *widget)
{
    int max = 0;

    for (int i = 0; i < 20; ++i)
    {
        const auto &[id, _] = widget->getMetatileOrigin(i);

        if (simple_tiles.contains(id))
        {
            const int l = simple_tiles.value(id).frames.length();

            if (l > max)
                max = l;
        }
    }

    return (max > 0)? max : 1;
}

//  TODO: factorise this ; keep functions with LOW number of parameters
void TileConverterDialog::onAddAutoTile()
{
    auto simple_tiles = simple_tiles_ptr.toStrongRef();
    Q_ASSERT(!simple_tiles.isNull());

    const int n_frames = get_n_frames(*simple_tiles, ui->metatilesViewWidget);
    QImage empty = QImage(tilesize, tilesize, QImage::Format_ARGB32_Premultiplied);
    empty.fill(Qt::transparent);

    AutoTile autotile;
    autotile.frames.resize(n_frames);

    QVector<QPair<QString, QRect>> origins(20);

//  the next frames depend on the previous frame
//  the first one is thus initialised a bit differently
    autotile.frames[0].metatiles.resize(20);
    for (int i = 0; i < 20; ++i)
    {
        origins[i] = ui->metatilesViewWidget->getMetatileOrigin(i);
        const auto &[id, rect] = origins[i];

        if (simple_tiles->contains(id))
        {
            const QImage tile = simple_tiles->value(id).frames[0];
            autotile.frames[0].metatiles[i] = tile.copy(rect);
        }
        else
        {
            autotile.frames[0].metatiles[i] = empty;
        }
    }

    for (int j = 1; j < n_frames; ++j)
    {
        autotile.frames[j].metatiles.resize(20);

        for (int i = 0; i < 20; ++i)
        {
            const auto &[id, rect] = ui->metatilesViewWidget->getMetatileOrigin(i);

            if (simple_tiles->contains(id))
            {
                const auto &frames = simple_tiles->value(id).frames;

                if (j < frames.length())
                    autotile.frames[j].metatiles[i] = frames[j].copy(rect);
                else
                    autotile.frames[j].metatiles[i] = autotile.frames[j-1].metatiles[i];
            }
            else
            {
                autotile.frames[j].metatiles[i] = autotile.frames[j-1].metatiles[i];
            }
        }
    }

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
//  see Types.hpp for why this represents a single isolated tile
    const Orientation single = {0, 3, 12, 15};
    ui->generatedAutoTilesWidget->addCaption(id, autotile.frames[0].genTile(single));
    added_autotiles.push_back({id, autotile});
    added_metatile_origins[id] = origins;
}

void TileConverterDialog::onRemoveAutoTile(const QString id)
{
    const int index = index_of(added_autotiles, id);
    Q_ASSERT(index >= 0);

    added_autotiles.remove(index);
    added_metatile_origins.remove(id);
    ui->generatedAutoTilesWidget->removeCaption(id);
}

void TileConverterDialog::updateMetatileSelectionWidget()
{
    auto autotiles = autotiles_ptr.toStrongRef();
    Q_ASSERT(!autotiles.isNull());

    Q_ASSERT(autotiles->contains(selected_autotile));
    const auto &frames = autotiles->value(selected_autotile).frames;
    Q_ASSERT(!frames.isEmpty());
    const auto &metatiles = frames[0].metatiles;

    for (int i = 0; i < metatiles.length(); ++i)
        ui->metatileSelectionWidget->setMetatile(i, metatiles[i].scaled(tilesize, tilesize));
}

void TileConverterDialog::onSelectedAutoTileChanged(const QString id)
{
    selected_autotile = id;

    updateMetatileSelectionWidget();

    ui->metatileSelectionWidget->setEnabled(true);
    ui->metatileSelectionWidget->update();

    onUpdateGeneratedSimpleTile();
}

void TileConverterDialog::onAddSimpleTile()
{
    const Orientation orientation = ui->metatileSelectionWidget->getOrientation();

    auto autotiles = autotiles_ptr.toStrongRef();
    Q_ASSERT(!autotiles.isNull());

    Q_ASSERT(autotiles->contains(selected_autotile));
    const auto &frames = autotiles->value(selected_autotile).frames;
    Q_ASSERT(!frames.isEmpty());

    SimpleTile tile;
    for (auto &frame: frames)
        tile.frames.push_back(frame.genTile(orientation));

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ui->generatedSimpleTilesWidget->addCaption(id, tile.frames[0]);
    added_simple_tiles.push_back({id, tile});
}

void TileConverterDialog::onRemoveSimpleTile(const QString id)
{
    const int index = index_of(added_simple_tiles, id);
    Q_ASSERT(index >= 0);

    added_simple_tiles.remove(index);
    ui->generatedSimpleTilesWidget->removeCaption(id);
}

void TileConverterDialog::onUpdateGeneratedSimpleTile()
{
    const QImage &assembled = ui->metatileSelectionWidget->assembleCaption();
    ui->generatedSimpleTilesWidget->setNewTileCaption(assembled);
}

void TileConverterDialog::updateSimpleTileSelectionWidget()
{
    auto simple_tiles_order = simple_tiles_order_ptr.toStrongRef();
    Q_ASSERT(!simple_tiles_order.isNull());
    auto simple_tiles = simple_tiles_ptr.toStrongRef();
    Q_ASSERT(!simple_tiles.isNull());

    for (auto &id: *simple_tiles_order)
    {
        Q_ASSERT(simple_tiles->contains(id));
        const auto &frames = simple_tiles->value(id).frames;
        Q_ASSERT(frames.length() > 0);

        ui->simpleTileSelectionWidget->addCaption(id, frames[0]);
    }

    ui->simpleTileSelectionWidget->setTilesize(tilesize);
    ui->simpleTileSelectionWidget->resize();
}

void TileConverterDialog::updateAutoTileSelectionWidget()
{
    auto autotiles_order = autotiles_order_ptr.toStrongRef();
    Q_ASSERT(!autotiles_order_ptr.isNull());
    auto autotiles = autotiles_ptr.toStrongRef();
    Q_ASSERT(!autotiles.isNull());

    for (auto &id: *autotiles_order)
    {
        Q_ASSERT(autotiles->contains(id));
        const auto &frames = autotiles->value(id).frames;
        Q_ASSERT(frames.length() > 0);
    //  see Types.hpp for why this is an isolated tile
        const QImage pixels = frames[0].genTile({0, 3, 12, 15});

        ui->autoTileSelectionWidget->addCaption(id, pixels);
    }

    ui->autoTileSelectionWidget->setTilesize(tilesize);
    ui->autoTileSelectionWidget->resize();
}

void TileConverterDialog::updateSelectionWidgets()
{
    updateSimpleTileSelectionWidget();

    ui->simpleTileViewWidget->setMetaTileSize(tilesize / 2);

    ui->metatilesViewWidget->setTilesize(tilesize);
    ui->metatilesViewWidget->resize();

    ui->generatedAutoTilesWidget->setTilesize(tilesize);
    ui->generatedAutoTilesWidget->resize();

    updateAutoTileSelectionWidget();

    ui->generatedSimpleTilesWidget->setTilesize(tilesize);
    ui->generatedSimpleTilesWidget->resize();

    ui->metatileSelectionWidget->setTilesize(tilesize);
    ui->metatileSelectionWidget->resize();
}

void TileConverterDialog::onAccept()
{
    if (!added_simple_tiles.isEmpty() || !added_autotiles.isEmpty())
    {
        auto undo_stack = undo_stack_ptr.toStrongRef();
        Q_ASSERT(!undo_stack.isNull());

        undo_stack->beginMacro("Tile Conversions");
        for (auto &[id, tile]: added_simple_tiles)
            undo_stack->push(new AddTileCommand<SimpleTile>(simple_tiles_order_ptr, simple_tiles_ptr, id, tile));
        for (auto &[id, tile]: added_autotiles)
            undo_stack->push(new AddAutoTileCommand(autotiles_order_ptr, autotiles_ptr, id, tile));
        undo_stack->endMacro();
    }

    accept();
}
