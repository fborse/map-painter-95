#include "FrameOrderEditorDialog.hpp"
#include "ui_FrameOrderEditorDialog.h"

#include <QPainter>
#include <QMouseEvent>

#include "AddRectDialog.hpp"

template <typename T>
static inline QSharedPointer<T> lock_ptr(QWeakPointer<T> &weak)
{
    auto shared = weak.toStrongRef();
    Q_ASSERT(!shared.isNull());

    return shared;
}

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
class FrameOrderReplaceTilesCommand final: public QUndoCommand, public TilesCommand<Tileset<T>>
{
public:
    FrameOrderReplaceTilesCommand(QWeakPointer<Tileset<T>> tiles, const QString &id, const T &prev, const T &next):
        QUndoCommand(), TilesCommand<Tileset<T>>(tiles), id{id}, prev{prev}, next{next}
    {}

    void undo() final override { (*TilesCommand<Tileset<T>>::lockTiles())[id] = prev; }
    void redo() final override { (*TilesCommand<Tileset<T>>::lockTiles())[id] = next; }

private:
    QString id;
    T prev, next;
};

FrameOrderTileSelectionWidget::FrameOrderTileSelectionWidget(QWidget *parent):
    QWidget(parent),
    tilesize{0},
    autotile_captions{}, simple_tile_captions{},
    n_columns{8}, selected_tile{},
    mouse_position{}
{
    setFixedSize(0, 0);
}

void FrameOrderTileSelectionWidget::addSimpleTileCaption(const QString &id, const QImage &caption)
{
    simple_tile_captions.push_back({id, caption});
}

void FrameOrderTileSelectionWidget::addAutoTileCaption(const QString &id, const QImage &caption)
{
    autotile_captions.push_back({id, caption});
}

QSize FrameOrderTileSelectionWidget::getGridAspect() const
{
    const int n_auto = autotile_captions.length();
    const int n_simple = simple_tile_captions.length();

    Q_ASSERT(n_columns > 0);
    const int h_auto = qCeil(n_auto / double(n_columns));
    const int h_simple = qCeil(n_simple / double(n_columns));

    return {n_columns, h_auto + h_simple};
}

void FrameOrderTileSelectionWidget::resize()
{
    setFixedSize(getGridAspect() * tilesize);
    update();
}

//  QPoint rounds by default ; here we truncate
static inline QPoint divide(const QPoint &p, const double f)
{
    return {int(p.x() / f), int(p.y() / f)};
}

std::optional<TileReference> FrameOrderTileSelectionWidget::toRef(const QPoint &ij) const
{
    const int index = ij.x() + ij.y() * n_columns;

    const int n_auto = autotile_captions.length();
    if (index < n_auto)
    {
        const QString id = autotile_captions[index].first;
        return {{id, true, {}}};
    }
    else
    {
        const int h_auto = qCeil(n_auto / double(n_columns));
        const int offset = h_auto * n_columns;
        const int idx = index - offset;
        if (idx < 0)
            return {};

        const int n_simple = simple_tile_captions.length();
        if (idx < n_simple)
        {
            const QString id = simple_tile_captions[idx].first;
            return {{id, false, {}}};
        }
    }

    return {};
}

void FrameOrderTileSelectionWidget::paintBackground(QPainter &painter)
{
    const auto &[w, h] = getGridAspect();
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

void FrameOrderTileSelectionWidget::paintCaptions(QPainter &painter)
{
    const int n_auto = autotile_captions.length();
    const int n_simple = simple_tile_captions.length();

    Q_ASSERT(n_columns > 0);
    const int h_auto = qCeil(n_auto / double(n_columns));

    for (int i = 0; i < n_auto; ++i)
    {
        const QPoint p(i % n_columns, i / n_columns);
        painter.drawImage(p * tilesize, autotile_captions[i].second);
    }

    for (int i = 0; i < n_simple; ++i)
    {
        const QPoint p(i % n_columns, i / n_columns + h_auto);
        painter.drawImage(p * tilesize, simple_tile_captions[i].second);
    }
}

void FrameOrderTileSelectionWidget::paintGrid(QPainter &painter)
{
    const auto &[w, h] = getGridAspect();

    const QColor white128 = {255, 255, 255, 128};

    for (int j = 0; j < h; ++j)
        painter.fillRect(0, j * tilesize, w * tilesize, 1, white128);
    for (int i = 0; i < w; ++i)
        painter.fillRect(i * tilesize, 0, 1, h * tilesize, white128);
    for (int j = 0; j < h; ++j)
        painter.fillRect(0, j * tilesize + tilesize-1, w * tilesize, 1, white128);
    for (int i = 0; i < w; ++i)
        painter.fillRect(i * tilesize + tilesize-1, 0, 1, h * tilesize, white128);
}

static inline int index_of(const QVector<QPair<QString, QImage>> &captions, const QString &id)
{
    for (int i = 0; i < captions.length(); ++i)
        if (captions[i].first == id)
            return i;

    return -1;
}

static inline void draw_cursor(QPainter &painter, const int i, const int j, const int unit)
{
    painter.setPen(Qt::black);
    painter.drawRect(i * unit, j * unit, unit - 1, unit - 1);
    painter.setPen(Qt::white);
    painter.drawRect(i * unit + 1, j * unit + 1, unit - 3, unit - 3);
}

void FrameOrderTileSelectionWidget::paintSelected(QPainter &painter)
{
    if (selected_tile.autotile && !selected_tile.name.isEmpty())
    {
        const int index = index_of(autotile_captions, selected_tile.name);
        Q_ASSERT(index >= 0);
        Q_ASSERT(n_columns > 0);
        const int i = index % n_columns, j = index / n_columns;

        draw_cursor(painter, i, j, tilesize);
    }
    else if (!selected_tile.autotile && !selected_tile.name.isEmpty())
    {
        const int index = index_of(simple_tile_captions, selected_tile.name);
        Q_ASSERT(index >= 0);
        Q_ASSERT(n_columns > 0);
        const int i = index % n_columns, j = index / n_columns;
        const int n = autotile_captions.length();
        const int h = qCeil(n / double(n_columns));

        draw_cursor(painter, i, j + h, tilesize);
    }
}

void FrameOrderTileSelectionWidget::paintHoverCursor(QPainter &painter)
{
    const QPoint p = divide(mouse_position, tilesize);
    const QRect r = {p * tilesize, QSize(tilesize, tilesize)};
    const bool valid = toRef(divide(mouse_position, tilesize)).has_value();

    const QColor white64 = {255, 255, 255, 64};
    const QColor red64 = {255, 0, 0, 64};

    painter.fillRect(r, valid? white64 : red64);
}

void FrameOrderTileSelectionWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    paintBackground(painter);
    paintCaptions(painter);
    paintGrid(painter);
    paintHoverCursor(painter);
    if (!selected_tile.isEmpty())
        paintSelected(painter);
}

void FrameOrderTileSelectionWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouse_position = event->pos();
    update();
}

void FrameOrderTileSelectionWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        if (const auto ref = toRef(divide(mouse_position, tilesize)))
            emit tileClicked(*ref);
}

FrameOrderEditorDialog::FrameOrderEditorDialog(QWidget *parent):
    QDialog(parent), ui(new Ui::FrameOrderEditorDialog),
    tilesize{0}, changed_simple_tiles{}, changed_autotiles{},
    undo_stack_ptr{nullptr},
    simple_tiles_order_ptr{nullptr}, autotiles_order_ptr{nullptr},
    simple_tiles_ptr{nullptr}, autotiles_ptr{nullptr},
    selected_tile{}, selected_frame{-1}
{
    ui->setupUi(this);

    ui->addFramePushButton->setEnabled(false);
    ui->cloneFramePushButton->setEnabled(false);
    ui->removeFramePushButton->setEnabled(false);
    ui->moveFrameUpPushButton->setEnabled(false);
    ui->moveFrameDownPushButton->setEnabled(false);
}

FrameOrderEditorDialog::~FrameOrderEditorDialog()
{
    delete ui;
}

template <typename T, class Fn>
static inline auto get_captions(QWeakPointer<Names> orders_ptr, QWeakPointer<Tileset<T>> tiles_ptr, Fn fn)
{
    auto orders = orders_ptr.toStrongRef();
    Q_ASSERT(!orders.isNull());
    auto tiles = tiles_ptr.toStrongRef();
    Q_ASSERT(!tiles.isNull());

    QVector<QPair<QString, QImage>> captions;

    for (auto &id: *orders)
    {
        Q_ASSERT(tiles->contains(id));
        const auto &frames = tiles->value(id).frames;
        Q_ASSERT(frames.length() > 0);

        captions.push_back({id, fn(frames[0])});
    }

    return captions;
}

void FrameOrderEditorDialog::updateTileset()
{
//  see Types.hpp for why this represents a single isolated tile
    const Orientation isolated = {0, 3, 12, 15};
    const auto autotiles = get_captions(autotiles_order_ptr, autotiles_ptr, [isolated]
        (const AutoTile::Frame &frame) { return frame.genTile(isolated); }
    );
    for (auto &[id, caption]: autotiles)
        ui->tileSelectionWidget->addAutoTileCaption(id, caption);

    const auto simple_tiles = get_captions(simple_tiles_order_ptr, simple_tiles_ptr, []
        (const QImage &caption) { return caption; }
    );
    for (auto &[id, caption]: simple_tiles)
        ui->tileSelectionWidget->addSimpleTileCaption(id, caption);

    ui->tileSelectionWidget->setTilesize(tilesize);
    ui->tileSelectionWidget->resize();
}

static inline bool operator==(const SimpleTile &tile1, const SimpleTile &tile2)
{
    return tile1.frames == tile2.frames;
}

static inline bool operator!=(const SimpleTile &tile1, const SimpleTile &tile2)
{
    return !(tile1 == tile2);
}

static inline bool operator==(const AutoTile &tile1, const AutoTile &tile2)
{
    if (tile1.frames.length() != tile2.frames.length())
        return false;

    for (int i = 0; i < tile1.frames.length(); ++i)
        if (tile1.frames[i].metatiles != tile2.frames[i].metatiles)
            return false;

    return true;
}

static inline bool operator!=(const AutoTile &tile1, const AutoTile &tile2)
{
    return !(tile1 == tile2);
}

template <typename T>
static inline auto get_prev_next(QWeakPointer<Tileset<T>> tiles_ptr, QHash<QString, T> &changes)
{
    auto tiles = tiles_ptr.toStrongRef();
    Q_ASSERT(!tiles.isNull());

    QHash<QString, T> prev, next;

    for (auto &id: changes.keys())
    {
        Q_ASSERT(tiles->contains(id));
        const auto &tile = tiles->value(id);

        if (tile != changes[id])
        {
            prev[id] = tile;
            next[id] = changes[id];
        }
    }

    return QPair<decltype(prev), decltype(next)>(prev, next);
}

void FrameOrderEditorDialog::onAccept()
{
    const auto &[prev_simple, next_simple] = get_prev_next(simple_tiles_ptr, changed_simple_tiles);
    const auto &[prev_auto, next_auto] = get_prev_next(autotiles_ptr, changed_autotiles);

    if (!prev_simple.isEmpty() || !prev_auto.isEmpty())
    {
        auto undo_stack = undo_stack_ptr.toStrongRef();
        Q_ASSERT(!undo_stack.isNull());

        undo_stack->beginMacro("Modify Frames");
        for (auto &id: prev_simple.keys())
            undo_stack->push(new FrameOrderReplaceTilesCommand<SimpleTile>(simple_tiles_ptr, id, prev_simple[id], next_simple[id]));
        for (auto &id: prev_auto.keys())
            undo_stack->push(new FrameOrderReplaceTilesCommand<AutoTile>(autotiles_ptr, id, prev_auto[id], next_auto[id]));
        undo_stack->endMacro();
    }

    accept();
}

static inline QPixmap gen_caption(const QVector<QImage> &metatiles)
{
    Q_ASSERT(metatiles.length() == 20);
    const int s = metatiles[0].width();

    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};

//  Modified RPG Maker scheme, here
//  0   1   2   3   16
//  4   5   6   7   17
//  8   9   10  11  18
//  12  13  14  15  19
    QPixmap pixmap(5 * s, 4 * s);

    QPainter painter(&pixmap);
//  start with the background ; offshooting by s
    for (int j = 0; j < 3; ++j)
    {
        for (int i = 0; i < 2; ++i)
        {
            painter.fillRect(i * 2*s, j * 2*s, s, s, dark);
            painter.fillRect(i * 2*s + s, j * 2*s, s, s, light);
            painter.fillRect(i * 2*s, j * 2*s + s, s, s, light);
            painter.fillRect(i * 2*s + s, j * 2*s + s, s, s, dark);
        }
    }
//  regular metatiles
    for (int i = 0; i < 16; ++i)
        painter.drawImage((i%4) * s, (i/4) * s, metatiles[i]);
//  joint metatiles
    for (int i = 0; i < 4; ++i)
        painter.drawImage(4 * s, i * s, metatiles[16 + i]);

    return pixmap;
}

static inline QPixmap gen_caption(const QImage &tile)
{
    const int s = tile.width() / 2;

    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};

    QPixmap pixmap(tile.size());

    QPainter painter(&pixmap);
    painter.fillRect(0, 0, s, s, dark);
    painter.fillRect(s, 0, s, s, light);
    painter.fillRect(0, s, s, s, light);
    painter.fillRect(s, s, s, s, dark);
    painter.drawImage(0, 0, tile);

    return pixmap;
}

void FrameOrderEditorDialog::onSelectedTileChanged(const TileReference ref)
{
    selected_tile = ref;

    ui->framesListWidget->clear();

    if (ref.autotile)
    {
        QVector<AutoTile::Frame> frames;
        if (changed_autotiles.contains(ref.name))
        {
            frames = changed_autotiles.value(ref.name).frames;
        }
        else
        {
            auto autotiles = autotiles_ptr.toStrongRef();
            Q_ASSERT(!autotiles.isNull());
            Q_ASSERT(autotiles->contains(ref.name));
            frames = autotiles->value(ref.name).frames;
        }

        for (auto &frame: frames)
        {
            QPixmap caption = gen_caption(frame.metatiles);
            ui->framesListWidget->setIconSize(caption.size());
            ui->framesListWidget->addItem(new QListWidgetItem(QIcon(caption), ""));
        }
    }
    else
    {
        ui->framesListWidget->setIconSize({tilesize, tilesize});

        QVector<QImage> frames;
        if (changed_simple_tiles.contains(ref.name))
        {
            frames = changed_simple_tiles.value(ref.name).frames;
        }
        else
        {
            auto simple_tiles = simple_tiles_ptr.toStrongRef();
            Q_ASSERT(!simple_tiles.isNull());
            Q_ASSERT(simple_tiles->contains(ref.name));
            frames = simple_tiles->value(ref.name).frames;
        }

        for (auto &original: frames)
        {
            QPixmap caption = gen_caption(original);
            ui->framesListWidget->setIconSize(caption.size());
            ui->framesListWidget->addItem(new QListWidgetItem(QIcon(caption), ""));
        }
    }

    ui->addFramePushButton->setEnabled(true);
}

void FrameOrderEditorDialog::onSelectedFrameChanged(const int index)
{
    const int n = ui->framesListWidget->count();

    ui->cloneFramePushButton->setEnabled(index >= 0);
    ui->removeFramePushButton->setEnabled((index >= 0) && (n > 1));
    ui->moveFrameUpPushButton->setEnabled(index > 0);
    ui->moveFrameDownPushButton->setEnabled((index >= 0) && (index < n-1));

    selected_frame = index;
}

void FrameOrderEditorDialog::addChangedSimpleTile(const QString &id)
{
    auto simple_tiles = simple_tiles_ptr.toStrongRef();
    Q_ASSERT(!simple_tiles.isNull());

    Q_ASSERT(simple_tiles->contains(id));
    for (auto &frame: simple_tiles->value(id).frames)
    //  .frames is already initialised by QHash's operator[]
        changed_simple_tiles[id].frames.push_back(frame);
}

void FrameOrderEditorDialog::addChangedAutoTile(const QString &id)
{
    auto autotiles = autotiles_ptr.toStrongRef();
    Q_ASSERT(!autotiles.isNull());

    Q_ASSERT(autotiles->contains(id));
    for (auto &frame: autotiles->value(id).frames)
    //  .frames is already initialised by QHash's operator[]
        changed_autotiles[id].frames.push_back(frame);
}

static inline QImage gen_image(const QSize &size, const QColor &color)
{
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(color);

    return image;
}

void FrameOrderEditorDialog::onAddFrame()
{
    AddRectDialog dialog(tilesize, this);
    dialog.setWindowTitle("Add Frame");
    if (dialog.exec() == QDialog::Accepted)
    {
        const int n = ui->framesListWidget->count();
        const int index = (selected_frame < 0)? n : selected_frame+1;

        QPixmap caption;

        const QString id = selected_tile.name;
        Q_ASSERT(!id.isEmpty());
        if (selected_tile.autotile)
        {
            QVector<QImage> metatiles(20);
            for (auto &metatile: metatiles)
                metatile = gen_image({tilesize/2, tilesize/2}, dialog.getColor());
            caption = gen_caption(metatiles);

            if (!changed_autotiles.contains(id))
                addChangedAutoTile(id);
            changed_autotiles[id].frames.insert(index, {metatiles});
        }
        else
        {
            QImage frame = gen_image({tilesize, tilesize}, dialog.getColor());
            caption = gen_caption(frame);

            if (!changed_simple_tiles.contains(id))
                addChangedSimpleTile(id);
            changed_simple_tiles[id].frames.insert(index, frame);
        }

        ui->framesListWidget->insertItem(index, new QListWidgetItem(QIcon(caption), ""));
        ui->framesListWidget->setCurrentRow(index);
    }
}

//  find a way to factorise the repeated code between the next few functions
void FrameOrderEditorDialog::onCloneFrame()
{
    Q_ASSERT(!selected_tile.name.isEmpty());
    Q_ASSERT(selected_frame >= 0);

    const QString id = selected_tile.name;
    Q_ASSERT(!id.isEmpty());
    if (selected_tile.autotile)
    {
        if (!changed_autotiles.contains(id))
            addChangedAutoTile(id);

        const int n = changed_autotiles[id].frames.length();
        Q_ASSERT(selected_frame < n);
        const auto &frame = changed_autotiles[id].frames[selected_frame];
        changed_autotiles[id].frames.insert(selected_frame + 1, frame);
    }
    else
    {
        if (!changed_simple_tiles.contains(id))
            addChangedSimpleTile(id);

        const int n = changed_simple_tiles[id].frames.length();
        Q_ASSERT(selected_frame < n);
        const auto &frame = changed_simple_tiles[id].frames[selected_frame];
        changed_simple_tiles[id].frames.insert(selected_frame + 1, frame);
    }

    QIcon caption = ui->framesListWidget->currentItem()->icon();
    ui->framesListWidget->insertItem(selected_frame + 1, new QListWidgetItem(caption, ""));
    ui->framesListWidget->setCurrentRow(selected_frame + 1);
}

void FrameOrderEditorDialog::onRemoveFrame()
{
    const int n = ui->framesListWidget->count();
    Q_ASSERT(0 <= selected_frame && selected_frame < n);

    const QString id = selected_tile.name;
    Q_ASSERT(!id.isEmpty());
    if (selected_tile.autotile)
    {
        if (!changed_autotiles.contains(id))
            addChangedAutoTile(id);

        const int n = changed_autotiles[id].frames.length();
        Q_ASSERT(selected_frame < n);
        changed_autotiles[id].frames.remove(selected_frame);
    }
    else
    {
        if (!changed_simple_tiles.contains(id))
            addChangedSimpleTile(id);

        const int n = changed_simple_tiles[id].frames.length();
        Q_ASSERT(selected_frame < n);
        changed_simple_tiles[id].frames.remove(selected_frame);
    }

    delete ui->framesListWidget->takeItem(selected_frame);
    onSelectedFrameChanged((selected_frame > 0)? selected_frame-1 : 0);
}

void FrameOrderEditorDialog::onMoveFrameUp()
{
    Q_ASSERT(selected_frame > 0);

    const QString id = selected_tile.name;
    Q_ASSERT(!id.isEmpty());
    if (selected_tile.autotile)
    {
        if (!changed_autotiles.contains(id))
            addChangedAutoTile(id);

        const int n = changed_autotiles[id].frames.length();
        Q_ASSERT(selected_frame < n);   //  no need to check selected_frame - 1
        changed_autotiles[id].frames.swapItemsAt(selected_frame - 1, selected_frame);
    }
    else
    {
        if (!changed_simple_tiles.contains(id))
            addChangedSimpleTile(id);

        const int n = changed_simple_tiles[id].frames.length();
        Q_ASSERT(selected_frame < n);   //  no need to check selected_frame - 1
        changed_simple_tiles[id].frames.swapItemsAt(selected_frame - 1, selected_frame);
    }

//  selected_frame changes when changing current row
    const int index = selected_frame;
    ui->framesListWidget->setCurrentRow(-1);
    auto *item = ui->framesListWidget->takeItem(index);
    ui->framesListWidget->insertItem(index - 1, item);

    ui->framesListWidget->setCurrentRow(index - 1);
}

void FrameOrderEditorDialog::onMoveFrameDown()
{
    const int n = ui->framesListWidget->count();
    Q_ASSERT(0 <= selected_frame && selected_frame < n - 1);

    const QString id = selected_tile.name;
    Q_ASSERT(!id.isEmpty());
    if (selected_tile.autotile)
    {
        if (!changed_autotiles.contains(id))
            addChangedAutoTile(id);

        const int n = changed_autotiles[id].frames.length();
        Q_ASSERT(selected_frame + 1 < n);   //  no need to check selected_frame too
        changed_autotiles[id].frames.swapItemsAt(selected_frame, selected_frame + 1);
    }
    else
    {
        if (!changed_simple_tiles.contains(id))
            addChangedSimpleTile(id);

        const int n = changed_simple_tiles[id].frames.length();
        Q_ASSERT(selected_frame + 1 < n);   //  no need to check selected_frame too
        changed_simple_tiles[id].frames.swapItemsAt(selected_frame, selected_frame + 1);
    }

//  selected_frame changes when changing current row
    const int index = selected_frame;
    ui->framesListWidget->setCurrentRow(-1);
    auto *item = ui->framesListWidget->takeItem(index);
    ui->framesListWidget->insertItem(index + 1, item);

    ui->framesListWidget->setCurrentRow(index + 1);
}
