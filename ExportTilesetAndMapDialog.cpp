#include "ExportTilesetAndMapDialog.hpp"
#include "ui_ExportTilesetAndMapDialog.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QPainter>

ExportTilesetAndMapDialog::ExportTilesetAndMapDialog(const int tilesize, QWidget *parent):
    QDialog(parent), ui(new Ui::ExportTilesetAndMapDialog),
    tilesize{tilesize}, tile_coordinates{}, drawn_tileset{},
    simple_tiles_order{nullptr}, simple_tiles{nullptr}, map_layers{nullptr}
{
    ui->setupUi(this);

    connect(ui->tilesetPathPushButton, &QPushButton::clicked, [&] {
        const char *title = "Choose a texture file to export the tileset to !";
        const char *ext = "Texture files (*.png)";
        const QString path = QFileDialog::getSaveFileName(this, tr(title), QString(), tr(ext));

        if (!path.isEmpty())
            ui->tilesetPathLineEdit->setText(path);
    });

    connect(ui->mapPathPushButton, &QPushButton::clicked, [&] {
        const char *title = "Choose a file to export the map to !";
        const char *ext = "Map files (*.map95)";
        const QString path = QFileDialog::getSaveFileName(this, tr(title), QString(), tr(ext));

        if (!path.isEmpty())
            ui->mapPathLineEdit->setText(path);
    });
}

ExportTilesetAndMapDialog::~ExportTilesetAndMapDialog()
{
    delete ui;
}

QString ExportTilesetAndMapDialog::getTilesetPath() const
{
    return ui->tilesetPathLineEdit->text();
}

QString ExportTilesetAndMapDialog::getMapPath() const
{
    return ui->mapPathLineEdit->text();
}

int ExportTilesetAndMapDialog::getNumberOfColumns() const
{
    return ui->numberOfColumnsSpinBox->value();
}

static inline void save_map(const QString &filename, const QHash<QString, QVector<QPoint>> &coords, const MapLayers &map_layers)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
        throw QString("Could not write to '%1'").arg(filename);

    QDataStream stream(&file);
    {
        uint w = 0, h = 0, n = 0;
        n = map_layers.length();
        if (n > 0)
            h = map_layers[0].length();
        if (n > 0 && h > 0)
            w = map_layers[0][0].length();

        stream << n << w << h;
    }

    for (auto &layer: map_layers)
        for (auto &row: layer)
            for (auto &ref: row)
                if (ref.isEmpty())
                    stream << uint(0) << uint(0);
                else
                    for (auto &p: coords[ref.name])
                        stream << uint(p.x()) << uint(p.y());
}

void ExportTilesetAndMapDialog::onAccept() try
{
    if (getTilesetPath().isEmpty())
        throw std::runtime_error("No path given for the tileset !");
    if (getMapPath().isEmpty())
        throw std::runtime_error("No path given for the map !");
    if (drawn_tileset.isNull())
        throw std::runtime_error("The image generated for the tileset is empty !");

    QSharedPointer<MapLayers> map_layers_ptr = map_layers.toStrongRef();
    Q_ASSERT(!map_layers_ptr.isNull());

    if (!drawn_tileset.save(getTilesetPath()))
        throw QString("Could not save the drawn tileset !");
    save_map(getMapPath(), tile_coordinates, *map_layers_ptr);

    accept();
}
catch (std::runtime_error &error)
{
    const char *title = "The tileset and the map cannot be exported yet !";
    QMessageBox::information(this, tr(title), tr(error.what()));
}
catch (const QString &errstr)
{
    const char *title = "The tileset or the map could not be exported !";
    QMessageBox::warning(this, tr(title), tr(errstr.toStdString().c_str()));
}

using OrderedSimpleTiles = QVector<QPair<QString, SimpleTile>>;

static inline OrderedSimpleTiles linearise_tiles(QWeakPointer<Names> tiles_order_ptr, QWeakPointer<SimpleTiles> simple_tiles_ptr)
{
    OrderedSimpleTiles linearised;

//  here tiles_order/tileset null is a legit case
    QSharedPointer<Names> tiles_order = tiles_order_ptr.toStrongRef();
    if (tiles_order.isNull())
        return linearised;
    QSharedPointer<SimpleTiles> simple_tiles = simple_tiles_ptr.toStrongRef();
    if (simple_tiles.isNull())
        return linearised;

    for (auto &id: *tiles_order)
        linearised.push_back({id, simple_tiles->value(id)});

    return linearised;
}

static inline int get_n_images(const OrderedSimpleTiles &tiles)
{
    int n = 0;

    for (auto &[_, tile]: tiles)
        n += tile.frames.length();

    return n;
}

static inline QPoint toIJ(const int index, const int n_columns)
{
    return {(index + 1) % n_columns, (index + 1) / n_columns};
}

//  both draws the tileset and creates and index of coordinates which the map refers to
static inline QImage gen_tileset(const OrderedSimpleTiles &tiles, const int ncol, const int tilesize, QHash<QString, QVector<QPoint>> &coords)
{
    const int n = get_n_images(tiles);
    const int h = qCeil((n + 1) / float(ncol));
    const int w = (n + 1 < ncol)? n + 1 : ncol;
    coords.clear();

    QImage tileset(w * tilesize, h * tilesize, QImage::Format_ARGB32_Premultiplied);
    tileset.fill(Qt::transparent);

    QPainter painter(&tileset);

    int i = 0;
    for (auto &[id, tile]: tiles)
    {
        for (auto &frame: tile.frames)
        {
            const QPoint p = toIJ(i, ncol) * tilesize;

            coords[id].push_back(p);
            painter.drawImage(p, frame);

            ++i;
        }
    }

    return tileset;
}

static inline QImage gen_background(const QSize &size, const int tilesize)
{
    QImage background(size, QImage::Format_ARGB32_Premultiplied);

    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};
    const int s = tilesize;

    QPainter painter(&background);
    for (int j = 0; j < size.height() / tilesize + 1; ++j)
    {
        for (int i = 0; i < size.width() / tilesize + 1; ++i)
        {
            painter.fillRect(i * s, j * s, s/2, s/2, dark);
            painter.fillRect(i * s + s/2, j * s, s/2, s/2, light);
            painter.fillRect(i * s, j * s + s/2, s/2, s/2, light);
            painter.fillRect(i * s + s/2, j * s + s/2, s/2, s/2, dark);
        }
    }

    return background;
}

void ExportTilesetAndMapDialog::redrawTileset()
{
    const auto tiles = linearise_tiles(simple_tiles_order, simple_tiles);

    if (!tiles.isEmpty())
    {
        drawn_tileset = gen_tileset(tiles, getNumberOfColumns(), tilesize, tile_coordinates);

        QImage generated = gen_background(drawn_tileset.size(), tilesize);
        QPainter painter(&generated);
        painter.drawImage(0, 0, drawn_tileset);

        ui->tilesetPreviewLabel->setPixmap(QPixmap::fromImage(generated));
    }
}
