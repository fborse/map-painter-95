#include "TileConverterDialog.hpp"
#include "ui_TileConverterDialog.h"

#include <QPainter>
#include <QMouseEvent>
#include <QUuid>

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
    added_simple_tiles{}, added_autotiles{}
{
    ui->setupUi(this);

    ui->metatileSelectionWidget->setEnabled(false);
}

TileConverterDialog::~TileConverterDialog()
{
    delete ui;
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
    updateAutoTileSelectionWidget();

    ui->generatedSimpleTilesWidget->setTilesize(tilesize);
    ui->generatedSimpleTilesWidget->resize();

    ui->metatileSelectionWidget->setTilesize(tilesize);
    ui->metatileSelectionWidget->resize();
}
