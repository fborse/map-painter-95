#include "FrameOrderEditorDialog.hpp"
#include "ui_FrameOrderEditorDialog.h"

#include <QPainter>
#include <QMouseEvent>

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
    tilesize{0},
    undo_stack_ptr{nullptr},
    simple_tiles_order_ptr{nullptr}, autotiles_order_ptr{nullptr},
    simple_tiles_ptr{nullptr}, autotiles_ptr{nullptr},
    selected_tile{}
{
    ui->setupUi(this);
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

void FrameOrderEditorDialog::onAccept()
{
    accepted();
}

void FrameOrderEditorDialog::onSelectedTileChanged(const TileReference ref)
{
    ui->framesListWidget->clear();

    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};

    if (ref.autotile)
    {
        const int s = tilesize/2;
    //  Modified RPG Maker scheme, here
    //  0   1   2   3   16
    //  4   5   6   7   17
    //  8   9   10  11  18
    //  12  13  14  15  19
        ui->framesListWidget->setIconSize({5 * s, 4 * s});

        auto autotiles = autotiles_ptr.toStrongRef();
        Q_ASSERT(!autotiles.isNull());
        Q_ASSERT(autotiles->contains(ref.name));
        const auto &frames = autotiles->value(ref.name).frames;

        for (auto &frame: frames)
        {
            QPixmap pixmap(5 * s, 4 * s);

            QPainter painter(&pixmap);
        //  background ; offshooting by tilesize/2 (aka s)
            for (int j = 0; j < 3; ++j)
            {
                for (int i = 0; i < 2; ++i)
                {
                    painter.fillRect(i * tilesize, j * tilesize, s, s, dark);
                    painter.fillRect(i * tilesize + s, j * tilesize, s, s, light);
                    painter.fillRect(i * tilesize, j * tilesize + s, s, s, light);
                    painter.fillRect(i * tilesize + s, j * tilesize + s, s, s, dark);
                }
            }
        //  "regular" metatiles
            for (int i = 0; i < 16; ++i)
                painter.drawImage((i%4) * s, (i/4) * s, frame.metatiles[i]);
        //  joint metatiles
            for (int i = 0; i < 4; ++i)
                painter.drawImage(4 * s, i * s, frame.metatiles[16 + i]);

            ui->framesListWidget->addItem(new QListWidgetItem(QIcon(pixmap), ""));
        }
    }
    else
    {
        ui->framesListWidget->setIconSize({tilesize, tilesize});

        auto simple_tiles = simple_tiles_ptr.toStrongRef();
        Q_ASSERT(!simple_tiles.isNull());
        Q_ASSERT(simple_tiles->contains(ref.name));
        const auto &frames = simple_tiles->value(ref.name).frames;

        for (auto &original: frames)
        {
        //  it's relatively cheap to just redraw the background for every frame
            QPixmap pixmap(tilesize, tilesize);

            QPainter painter(&pixmap);
            painter.fillRect(0, 0, tilesize/2, tilesize/2, dark);
            painter.fillRect(tilesize/2, 0, tilesize/2, tilesize/2, light);
            painter.fillRect(0, tilesize/2, tilesize/2, tilesize/2, dark);
            painter.fillRect(tilesize/2, tilesize/2, tilesize/2, tilesize/2, dark);
            painter.drawImage(0, 0, original);

            ui->framesListWidget->addItem(new QListWidgetItem(QIcon(pixmap), ""));
        }
    }
}
