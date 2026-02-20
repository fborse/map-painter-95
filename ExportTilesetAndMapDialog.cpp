#include "ExportTilesetAndMapDialog.hpp"
#include "ui_ExportTilesetAndMapDialog.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QPainter>

ExportTilesetAndMapDialog::ExportTilesetAndMapDialog(const int tilesize, QWidget *parent):
    QDialog(parent), ui(new Ui::ExportTilesetAndMapDialog),
    tilesize{tilesize},
    tiles_order{nullptr}, tileset{nullptr}, map_layers{nullptr}
{
    ui->setupUi(this);
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

//  TODO: create a spinbox for that
int ExportTilesetAndMapDialog::getNumberOfColumns() const
{
    return 8;
}

void ExportTilesetAndMapDialog::onAccept() try
{
    if (getTilesetPath().isEmpty())
        throw std::runtime_error("No path given for the tileset !");
    if (getMapPath().isEmpty())
        throw std::runtime_error("No path given for the map !");

    accept();
}
catch (std::runtime_error &error)
{
    const char *title = "The tileset and the map cannot be exported yet !";
    QMessageBox::information(this, tr(title), tr(error.what()));
}

static inline QVector<QImage> linearise_tiles(QWeakPointer<Names> tiles_order_ptr, QWeakPointer<Tileset> tileset_ptr)
{
    QVector<QImage> linearised;

    QSharedPointer<Names> tiles_order = tiles_order_ptr.toStrongRef();
    if (tiles_order.isNull())
        return linearised;
    QSharedPointer<Tileset> tileset = tileset_ptr.toStrongRef();
    if (tileset.isNull())
        return linearised;

    for (auto &id: *tiles_order)
        linearised.push_back(tileset->value(id).copy());

    return linearised;
}

static inline QPoint toIJ(const int index, const int n_columns)
{
    return {(index + 1) % n_columns, (index + 1) / n_columns};
}

void ExportTilesetAndMapDialog::redrawTileset()
{
    const QVector<QImage> tiles = linearise_tiles(tiles_order, tileset);
    const int ncol = getNumberOfColumns();

    const int n = qCeil((tiles.length() + 1) / float(ncol));

    QImage generated(ncol * tilesize, n * tilesize, QImage::Format_ARGB32_Premultiplied);
    generated.fill(Qt::transparent);
    {
        QPainter painter(&generated);

        for (int i = 0; i < tiles.length(); ++i)
            painter.drawImage(toIJ(i, ncol) * tilesize, tiles[i]);
    }

    ui->tilesetPreviewLabel->setPixmap(QPixmap::fromImage(generated));
}
