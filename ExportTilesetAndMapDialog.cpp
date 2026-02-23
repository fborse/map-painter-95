#include "ExportTilesetAndMapDialog.hpp"
#include "ui_ExportTilesetAndMapDialog.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QPainter>

ExportTilesetAndMapDialog::ExportTilesetAndMapDialog(const int tilesize, QWidget *parent):
    QDialog(parent), ui(new Ui::ExportTilesetAndMapDialog),
    tilesize{tilesize}, tile_coordinates{}, drawn_tileset{},
    tiles_order{nullptr}, tileset{nullptr}, map_layers{nullptr}
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

static inline void save_map(const QString &filename, const QHash<QString, QPoint> &coords, const MapLayer &map_layers)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
        throw QString("Could not write to '%1'").arg(filename);

    QDataStream stream(&file);
    {
        uint w = 0, h = 0;
        h = map_layers.length();
        if (map_layers.length() > 0)
            w = map_layers[0].length();

        stream << w << h;
    }

    for (auto &row: map_layers)
        for (auto &id: row)
            if (id.isEmpty())
                stream << uint(0) << uint(0);
            else
                stream << uint(coords[id].x()) << uint(coords[id].y());
}

void ExportTilesetAndMapDialog::onAccept() try
{
    if (getTilesetPath().isEmpty())
        throw std::runtime_error("No path given for the tileset !");
    if (getMapPath().isEmpty())
        throw std::runtime_error("No path given for the map !");
    if (drawn_tileset.isNull())
        throw std::runtime_error("The image generated for the tileset is empty !");

    QSharedPointer<MapLayer> map_layers_ptr = map_layers.toStrongRef();
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

using OrderedTileset = QVector<QPair<QString, QImage>>;

static inline OrderedTileset linearise_tiles(QWeakPointer<Names> tiles_order_ptr, QWeakPointer<Tileset> tileset_ptr)
{
    OrderedTileset linearised;

    QSharedPointer<Names> tiles_order = tiles_order_ptr.toStrongRef();
    if (tiles_order.isNull())
        return linearised;
    QSharedPointer<Tileset> tileset = tileset_ptr.toStrongRef();
    if (tileset.isNull())
        return linearised;

    for (auto &id: *tiles_order)
        linearised.push_back({id, tileset->value(id).copy()});

    return linearised;
}

static inline QPoint toIJ(const int index, const int n_columns)
{
    return {(index + 1) % n_columns, (index + 1) / n_columns};
}

static inline QImage gen_tileset(const OrderedTileset &tiles, const int ncol, const int tilesize, QHash<QString, QPoint> &coords)
{
    const int n = qCeil((tiles.length() + 1) / float(ncol));
    const int w = (tiles.length() + 1 < ncol)? tiles.length() + 1 : ncol;
    coords.clear();

    QImage tileset(w * tilesize, n * tilesize, QImage::Format_ARGB32_Premultiplied);
    tileset.fill(Qt::transparent);

    QPainter painter(&tileset);
    for (int i = 0; i < tiles.length(); ++i)
    {
        const QPoint p = toIJ(i, ncol) * tilesize;

        coords[tiles[i].first] = p;
        painter.drawImage(p, tiles[i].second);
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
    const auto tiles = linearise_tiles(tiles_order, tileset);

    if (!tiles.isEmpty())
    {
        drawn_tileset = gen_tileset(tiles, getNumberOfColumns(), tilesize, tile_coordinates);

        QImage generated = gen_background(drawn_tileset.size(), tilesize);
        QPainter painter(&generated);
        painter.drawImage(0, 0, drawn_tileset);

        ui->tilesetPreviewLabel->setPixmap(QPixmap::fromImage(generated));
    }
}
