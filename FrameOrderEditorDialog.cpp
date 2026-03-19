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

void FrameOrderTileSelectionWidget::paintSelected(QPainter &painter)
{
    if (selected_tile.autotile && !selected_tile.name.isEmpty())
    {
        const int index = index_of(autotile_captions, selected_tile.name);
        Q_ASSERT(index >= 0);
        Q_ASSERT(n_columns > 0);
        const int x = index % n_columns, y = index / n_columns;

        painter.setPen(Qt::black);
        painter.drawRect(x * tilesize, y * tilesize, tilesize - 1, tilesize - 1);
        painter.setPen(Qt::white);
        painter.drawRect(x * tilesize + 1, y * tilesize + 1, tilesize - 3, tilesize - 3);
    }
    else if (!selected_tile.autotile && !selected_tile.name.isEmpty())
    {
        const int index = index_of(simple_tile_captions, selected_tile.name);
        Q_ASSERT(index >= 0);
        Q_ASSERT(n_columns > 0);
        const int x = index % n_columns, y = index / n_columns;
        const int n = autotile_captions.length();
        const int h = qCeil(n / double(n_columns));

        painter.setPen(Qt::black);
        painter.drawRect(x * tilesize, (y+h) * tilesize, tilesize - 1, tilesize - 1);
        painter.setPen(Qt::white);
        painter.drawRect(x * tilesize + 1, (y+h) * tilesize + 1, tilesize - 3, tilesize - 3);
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

void FrameOrderEditorDialog::updateTileset()
{
    auto simple_tiles_order = simple_tiles_order_ptr.toStrongRef();
    Q_ASSERT(!simple_tiles_order.isNull());
    auto simple_tiles = simple_tiles_ptr.toStrongRef();
    Q_ASSERT(!simple_tiles.isNull());
    auto autotiles_order = autotiles_order_ptr.toStrongRef();
    Q_ASSERT(!autotiles_order.isNull());
    auto autotiles = autotiles_ptr.toStrongRef();
    Q_ASSERT(!autotiles.isNull());

//  see Types.hpp for why this represents a single isolated tile
    const Orientation isolated = {0, 3, 12, 15};
    for (auto &id: *autotiles_order)
    {
        Q_ASSERT(autotiles->contains(id));
        const auto &frames = autotiles->value(id).frames;
        Q_ASSERT(frames.length() > 0);
        ui->tileSelectionWidget->addAutoTileCaption(id, frames[0].genTile(isolated));
    }

    for (auto &id: *simple_tiles_order)
    {
        Q_ASSERT(simple_tiles->contains(id));
        const auto &frames = simple_tiles->value(id).frames;
        Q_ASSERT(frames.length() > 0);
        ui->tileSelectionWidget->addSimpleTileCaption(id, frames[0]);
    }

    ui->tileSelectionWidget->setTilesize(tilesize);
    ui->tileSelectionWidget->resize();
}

void FrameOrderEditorDialog::onAccept()
{
    accepted();
}

void FrameOrderEditorDialog::onSelectedTileChanged(const TileReference /*ref*/)
{}
