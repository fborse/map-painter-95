#include "ImportTilesetDialog.hpp"
#include "ui_ImportTilesetDialog.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QPainter>

ImportTilesetDialog::ImportTilesetDialog(QWidget *parent):
    QDialog(parent), ui(new Ui::ImportTilesetDialog),
    tilesize{-1},
    simple_tiles_order{}, autotiles_order{},
    simple_tiles{}, autotiles{},
    added_simple_tiles{}, added_autotiles{}
{
    ui->setupUi(this);

    ui->autoTilesListWidget->setEnabled(false);
    ui->simpleTilesListWidget->setEnabled(false);

    ui->addAutoTilePushButton->setEnabled(false);
    ui->addSimpleTilePushButton->setEnabled(false);

    ui->removeAutoTilePushButton->setEnabled(false);
    ui->removeSimpleTilePushButton->setEnabled(false);

    connect(ui->autoTilesListWidget, &QListWidget::currentRowChanged, [&] (const int index) {
        ui->addAutoTilePushButton->setEnabled(index >= 0);
    });
    connect(ui->simpleTilesListWidget, &QListWidget::currentRowChanged, [&] (const int index) {
        ui->addSimpleTilePushButton->setEnabled(index >= 0);
    });

    connect(ui->importedAutoTilesListWidget, &QListWidget::currentRowChanged, [&] (const int index) {
        ui->removeAutoTilePushButton->setEnabled(index >= 0);
    });
    connect(ui->importedSimpleTilesListWidget, &QListWidget::currentRowChanged, [&] (const int index) {
        ui->removeSimpleTilePushButton->setEnabled(index >= 0);
    });
}

ImportTilesetDialog::~ImportTilesetDialog()
{
    delete ui;
}

static inline QDataStream &operator>>(QDataStream &stream, AutoTile::Frame &frame)
{
    stream >> frame.metatiles;

    return stream;
}

struct TilesetInfo
{
    int tilesize;


    Names simple_tiles_order, autotiles_order;
    SimpleTiles simple_tiles;
    AutoTiles autotiles;

    void loadHeader(QDataStream &stream)
    {
        int major, minor;
        stream >> major >> minor;

        stream >> tilesize;
        if (tilesize < 1 || tilesize > 256)
            throw QString("Invalid tilesize %1 !").arg(tilesize);

        int n_columns, n_autos, n_simples;
        stream >> n_columns >> n_autos >> n_simples;
        if (n_columns < 1 || n_columns > 16)
            throw QString("Invalid number of columns %1 !").arg(n_columns);
        if (n_autos < 0)
            throw QString("Invalid number of autotiles %1 !").arg(n_autos);
        if (n_simples < 0)
            throw QString("Invalid number of tiles %1 !").arg(n_simples);

        autotiles_order.resize(n_autos);
        simple_tiles_order.resize(n_simples);
    }

    void loadAutoTiles(QDataStream &stream)
    {
        for (int i = 0; i < autotiles_order.length(); ++i)
        {
            QString &id = autotiles_order[i];
            stream >> id;
        //  QHash's operator[] automatically creates the tile
            stream >> autotiles[id].frames;
        }
    }

    void loadSimpleTiles(QDataStream &stream)
    {
        for (int i = 0; i < simple_tiles_order.length(); ++i)
        {
            QString &id = simple_tiles_order[i];
            stream >> id;
        //  QHash's operator[] automatically creates the tile
            stream >> simple_tiles[id].frames;
        }
    }

    static TilesetInfo load(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            throw QString("Could not read from '%1' !").arg(path);

        QDataStream stream(&file);

        TilesetInfo loaded;
        loaded.loadHeader(stream);
        loaded.loadAutoTiles(stream);
        loaded.loadSimpleTiles(stream);

        return loaded;
    }
};

void ImportTilesetDialog::onChangePath()
{
    if (!added_autotiles.isEmpty() || !added_simple_tiles.isEmpty())
    {
        const char *title = "Destructive operation ahead !";
        const char *msg = "You already have selected tiles from another tileset. "
                          "Changing to another tileset will reset the selected tiles for import. "
                          "Proceed anyway ?";
        const auto answer = QMessageBox::question(this, tr(title), tr(msg));
        if (answer == QMessageBox::No)
            return;

        added_autotiles.clear();
        added_simple_tiles.clear();
        ui->importedAutoTilesListWidget->clear();
        ui->importedSimpleTilesListWidget->clear();
    }

    const char *title = "Choose a Map Painter 95 file to import a tileset from !";
    const char *ext = "Map Painter 95 files (*.mp95)";
    const QString path = QFileDialog::getOpenFileName(this, tr(title), QString(), tr(ext));

    if (!path.isEmpty()) try
    {
        TilesetInfo tileset = TilesetInfo::load(path);
        if (tileset.tilesize != tilesize)
            throw std::runtime_error("Mismatch in tile sizes !");

        ui->pathLineEdit->setText(path);
        ui->autoTilesListWidget->setEnabled(true);
        ui->simpleTilesListWidget->setEnabled(true);

        simple_tiles_order = std::move(tileset.simple_tiles_order);
        autotiles_order = std::move(tileset.autotiles_order);
        simple_tiles = std::move(tileset.simple_tiles);
        autotiles = std::move(tileset.autotiles);

        updateAutoTilesSourceList();
        updateSimpleTilesSourceList();
    }
    catch (std::runtime_error &error)
    {
        QMessageBox::warning(this, tr("Could not load the tileset information !"), tr(error.what()));
    }
}

template <typename T>
static inline Tileset<T> get_tiles(const QVector<QString> &added, const Tileset<T> tiles)
{
    Tileset<T> added_tiles;

    for (auto &id: added)
    {
        Q_ASSERT(tiles.contains(id));
        added_tiles[id] = tiles[id];
    }

    return added_tiles;
}

AutoTiles ImportTilesetDialog::getAddedAutoTiles() const
{
    return get_tiles(added_autotiles, autotiles);
}

SimpleTiles ImportTilesetDialog::getAddedSimpleTiles() const
{
    return get_tiles(added_simple_tiles, simple_tiles);
}

void ImportTilesetDialog::onAddAutoTiles()
{
    for (auto *item: ui->autoTilesListWidget->selectedItems())
    {
        const int index = ui->autoTilesListWidget->row(item);
        Q_ASSERT(0 <= index && index < autotiles_order.length());
        added_autotiles.push_back(autotiles_order[index]);

        ui->importedAutoTilesListWidget->addItem(new QListWidgetItem(item->icon(), ""));
    }

    ui->autoTilesListWidget->setCurrentRow(-1);
}

void ImportTilesetDialog::onAddSimpleTiles()
{
    for (auto *item: ui->simpleTilesListWidget->selectedItems())
    {
        const int index = ui->simpleTilesListWidget->row(item);
        Q_ASSERT(0 <= index && index < simple_tiles_order.length());
        added_simple_tiles.push_back(simple_tiles_order[index]);

        ui->importedSimpleTilesListWidget->addItem(new QListWidgetItem(item->icon(), ""));
    }

    ui->simpleTilesListWidget->setCurrentRow(-1);
}

void ImportTilesetDialog::onRemoveAutoTiles()
{
    const auto items = ui->importedAutoTilesListWidget->selectedItems();

    for (auto it = items.crbegin(); it != items.crend(); ++it)
    {
        const int index = ui->importedAutoTilesListWidget->row(*it);
        Q_ASSERT(0 <= index && index < added_autotiles.length());
        added_autotiles.remove(index);

        delete *it;
    }

    ui->importedAutoTilesListWidget->setCurrentRow(-1);
}

void ImportTilesetDialog::onRemoveSimpleTiles()
{
    const auto items = ui->importedSimpleTilesListWidget->selectedItems();

    for (auto it = items.crbegin(); it != items.crend(); ++it)
    {
        const int index = ui->importedSimpleTilesListWidget->row(*it);
        Q_ASSERT(0 <= index && index < added_simple_tiles.length());
        added_simple_tiles.remove(index);

        delete *it;
    }

    ui->importedSimpleTilesListWidget->setCurrentRow(-1);
}

void ImportTilesetDialog::onAccept()
{
    if (added_autotiles.isEmpty() && added_simple_tiles.isEmpty())
    {
        const char *title = "Cannot import the tiles yet !";
        const char *msg = "You have not selected any tile for import !";
        QMessageBox::information(this, tr(title), tr(msg));
    }
    else
    {
        accept();
    }
}

static inline QPixmap gen_background(const QSize &aspect, const int tilesize)
{
    QPixmap pixmap(aspect * tilesize);
    QPainter painter(&pixmap);

    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};
    const int s = tilesize;

    for (int j = 0; j < aspect.height(); ++j)
    {
        for (int i = 0; i < aspect.width(); ++i)
        {
            painter.fillRect(i * s, j * s, s/2, s/2, dark);
            painter.fillRect(i * s + s/2, j * s, s/2, s/2, light);
            painter.fillRect(i * s, j * s + s/2, s/2, s/2, light);
            painter.fillRect(i * s + s/2, j * s + s/2, s/2, s/2, dark);
        }
    }

    return pixmap;
}

//  0   1   2   3   16
//  4   5   6   7   17
//  8   9   10  11  18
//  12  13  14  15  19
static inline QPixmap gen_autotile(const AutoTile &tile)
{
    Q_ASSERT(tile.frames.length() > 0);
    const auto &metatiles = tile.frames[0].metatiles;
//  see Types.hpp for why 20
    Q_ASSERT(metatiles.length() == 20);
    const int s = metatiles[0].width();

    QPixmap pixmap = gen_background({5, 4}, s);
    QPainter painter(&pixmap);

    for (int i = 0; i < metatiles.length(); ++i)
        painter.drawImage((i%4) * s, (i/4) * s, metatiles[i]);
    for (int j = 0; j < 4; ++j)
        painter.drawImage(4 * s, j * s, metatiles[16 + j]);

    return pixmap;
}

void ImportTilesetDialog::updateAutoTilesSourceList()
{
    ui->autoTilesListWidget->clear();
    ui->autoTilesListWidget->setIconSize({5*tilesize/2, 4*tilesize/2});
    ui->importedAutoTilesListWidget->setIconSize({5*tilesize/2, 4*tilesize/2});

    for (auto &id: autotiles_order)
    {
        Q_ASSERT(autotiles.contains(id));
        const QIcon icon = gen_autotile(autotiles[id]);

        ui->autoTilesListWidget->addItem(new QListWidgetItem(icon, ""));
    }
}

static inline QPixmap gen_simple_tile(const SimpleTile &tile)
{
    Q_ASSERT(tile.frames.length() > 0);
    const int s = tile.frames[0].width();

    QPixmap pixmap = gen_background({1, 1}, s);

    QPainter painter(&pixmap);
    painter.drawImage(0, 0, tile.frames[0]);

    return pixmap;
}

void ImportTilesetDialog::updateSimpleTilesSourceList()
{
    ui->simpleTilesListWidget->clear();
    ui->simpleTilesListWidget->setIconSize({tilesize, tilesize});
    ui->importedSimpleTilesListWidget->setIconSize({tilesize, tilesize});

    for (auto &id: simple_tiles_order)
    {
        Q_ASSERT(simple_tiles.contains(id));
        const QIcon icon = gen_simple_tile(simple_tiles[id]);

        ui->simpleTilesListWidget->addItem(new QListWidgetItem(icon, ""));
    }
}
