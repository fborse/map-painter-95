#include "ExportAsTexturesDialog.hpp"
#include "ui_ExportAsTexturesDialog.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QPainter>

ExportAsTexturesDialog::ExportAsTexturesDialog(const int tilesize, QWidget *parent):
    QDialog(parent), ui(new Ui::ExportAsTexturesDialog),
    tilesize{tilesize}, current_layer{0}, current_frame{0}, drawn_textures{},
    simple_tiles{nullptr}, map_layers{nullptr}
{
    ui->setupUi(this);

    connect(ui->directoryPushButton, &QPushButton::clicked, [&] {
        const char *title = "Choose a directory where the texture layers will be exported to !";
        const QString path = QFileDialog::getExistingDirectory(this, tr(title));

        if (!path.isEmpty())
            ui->directoryLineEdit->setText(path);
    });
}

ExportAsTexturesDialog::~ExportAsTexturesDialog()
{
    delete ui;
}

void ExportAsTexturesDialog::setMapLayersPointer(QWeakPointer<MapLayers> ptr)
{
    map_layers = ptr;

    redrawTextures();
    updateLayersComboBox();
    updateFramesComboBox();     //  couldn't call this from setTilesetPointer
    updateDisplayedTexture();
}

QString ExportAsTexturesDialog::getDirectory() const
{
    return ui->directoryLineEdit->text();
}

QString ExportAsTexturesDialog::getPattern() const
{
    return ui->namePatternLineEdit->text();
}

int ExportAsTexturesDialog::getNumberOfLayers() const
{
    auto map_layers_ptr = map_layers.toStrongRef();
    Q_ASSERT(!map_layers_ptr.isNull());

    return map_layers_ptr->length();
}

static inline int product(const QSet<int> &xs)
{
    int prod = 1;

    for (auto &x: xs)
        prod *= x;

    return prod;
}

static inline bool all_divisible_by(const QSet<int> &xs, const int y)
{
    for (auto &x: xs)
        if (y % x != 0)
            return false;

    return true;
}

static inline int naive_ppmc(const QSet<int> &xs)
{
    const int max = product(xs);
    for (int i = 1; i < max; ++i)
        if (all_divisible_by(xs, i))
            return i;

    return max;
}

static inline QSet<int> get_unique_lengths(const SimpleTiles &simple_tiles, const MapLayer &layer)
{
    QSet<int> lengths;

    for (auto &row: layer)
        for (auto &ref: row)
            if (ref.autotile)
            {}
            else
            {
                if (simple_tiles.contains(ref.name))
                    lengths.insert(simple_tiles[ref.name].frames.length());
            }

    return lengths;
}

int ExportAsTexturesDialog::getNumberOfFrames() const
{
    auto tileset_ptr = simple_tiles.toStrongRef();
    Q_ASSERT(!tileset_ptr.isNull());
    auto map_layers_ptr = map_layers.toStrongRef();
    Q_ASSERT(!map_layers_ptr.isNull());

    QSet<int> lengths;
    for (auto &layer: *map_layers_ptr)
        lengths += get_unique_lengths(*tileset_ptr, layer);

    return naive_ppmc(lengths);
}

void ExportAsTexturesDialog::onAccept() try
{
    if (getDirectory().isEmpty())
        throw std::runtime_error("The directory where to export has not been given yet !");
    if (getPattern().isEmpty())
        throw std::runtime_error("The file name pattern is empty !");
    if (getPattern().count('#') == 0)
        throw std::runtime_error("The file name pattern does not contain a layer number placeholder ! ('#')");
    if (getPattern().count('#') > 1)
        throw std::runtime_error("The file name pattern contains more than one layer number placeholder ! ('#')");
    if (getPattern().count('%') == 0)
        throw std::runtime_error("The file name pattern does not contain a frame number placeholder ! ('%')");
    if (getPattern().count('%') > 1)
        throw std::runtime_error("The file name pattern contains more than one frame number placeholder ! ('%')");

    auto tileset_ptr = simple_tiles.toStrongRef();
    Q_ASSERT(!tileset_ptr.isNull());
    auto map_layers_ptr = map_layers.toStrongRef();
    Q_ASSERT(!map_layers_ptr.isNull());
    Q_ASSERT(!map_layers_ptr->isEmpty());

    for (int l = 0; l < getNumberOfLayers(); ++l)
    {
        Q_ASSERT(!map_layers_ptr->at(l).isEmpty());
        const int n_frames = naive_ppmc(get_unique_lengths(*tileset_ptr, map_layers_ptr->at(l)));

        for (int f = 0; f < n_frames; ++f)
        {
            const QDir dir(getDirectory());
            QString pattern = getPattern();
            pattern.replace('#', QString::number(l + 1));
            pattern.replace('%', QString::number(f + 1));
            const QString path = dir.absoluteFilePath(pattern);

        //  TODO: provide a replace/discard/rename/cancel dialog
/*            if (dir.exists(pattern))
                throw QString("A file with that name exists already !");
            else*/ if (!drawn_textures[l][f].save(path))
                throw QString("Could not save to '%1' !").arg(path);
        }
    }

    accept();
}
catch (std::runtime_error &error)
{
    const char *title = "Cannot export the textures yet !";
    QMessageBox::information(this, tr(title), tr(error.what()));
}
catch (const QString &errstr)
{
    const char *title = "Could not export the textures !";
    QMessageBox::warning(this, tr(title), tr(errstr.toStdString().c_str()));
}

void ExportAsTexturesDialog::setCurrentLayer(const int layer)
{
    current_layer = layer;

    updateFramesComboBox();
    updateDisplayedTexture();
}

void ExportAsTexturesDialog::setCurrentFrame(const int frame)
{
    current_frame = frame;
    updateDisplayedTexture();
}

static inline QImage gen_layer(const int tilesize, const SimpleTiles &simple_tiles, const MapLayer &layer, const int frame)
{
    const int h = layer.length();
    if (layer.isEmpty())
        return {};
    const int w = layer[0].length();

    QImage image(w * tilesize, h * tilesize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    for (int j = 0; j < h; ++j)
    {
        for (int i = 0; i < w; ++i)
        {
            if (const auto ref = layer[j][i])
            {
                if (ref.autotile)
                {}
                else
                {
                    const auto &frames = simple_tiles[ref.name].frames;
                    const int n = frames.length();
                    painter.drawImage(i * tilesize, j * tilesize, frames[qMin(frame, n-1)]);
                }
            }
        }
    }

    return image;
}

void ExportAsTexturesDialog::redrawTextures()
{
    QSharedPointer<SimpleTiles> simple_tiles_ptr = simple_tiles.toStrongRef();
    Q_ASSERT(!simple_tiles_ptr.isNull());
    QSharedPointer<MapLayers> map_layers_ptr = map_layers.toStrongRef();
    Q_ASSERT(!map_layers_ptr.isNull());

    const int n_layers = map_layers_ptr->length();
    for (int l = 0; l < n_layers; ++l)
    {
        const auto &layer = map_layers_ptr->at(l);
        Q_ASSERT(!layer.isEmpty());
        const int n_frames = naive_ppmc(get_unique_lengths(*simple_tiles_ptr, layer));

        QVector<QImage> drawn_frames;
        for (int f = 0; f < n_frames; ++f)
            drawn_frames.push_back(gen_layer(tilesize, *simple_tiles_ptr, layer, f));
        drawn_textures.push_back(std::move(drawn_frames));
    }
}

void ExportAsTexturesDialog::updateLayersComboBox()
{
    Q_ASSERT(drawn_textures.length() > 0);

    ui->currentLayerComboBox->blockSignals(true);
    ui->currentLayerComboBox->clear();
    for (int l = 0; l < drawn_textures.length(); ++l)
        ui->currentLayerComboBox->addItem(QString::number(l + 1));
    ui->currentLayerComboBox->blockSignals(false);
}

//  assumption is that all xs are > 0
static inline int max_of(const QSet<int> &xs)
{
    int max = 0;

    for (auto &x: xs)
        if (max < x)
            max = x;

    return max;
}

void ExportAsTexturesDialog::updateFramesComboBox()
{
    auto simple_tiles_ptr = simple_tiles.toStrongRef();
    Q_ASSERT(!simple_tiles_ptr.isNull());
    auto map_layers_ptr = map_layers.toStrongRef();
    Q_ASSERT(!map_layers_ptr.isNull());

    const int n = max_of(get_unique_lengths(*simple_tiles_ptr, map_layers_ptr->value(current_layer)));

    ui->currentFrameComboBox->blockSignals(true);
    ui->currentFrameComboBox->clear();
    for (int f = 0; f < n; ++f)
        ui->currentFrameComboBox->addItem(QString::number(f + 1));
    ui->currentFrameComboBox->blockSignals(false);

    if (current_frame >= n)
        setCurrentFrame(n - 1);
}

static inline QImage gen_background(const QSize &size, const int tilesize)
{
    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};
    const int s = tilesize;

    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&image);
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

    return image;
}

void ExportAsTexturesDialog::updateDisplayedTexture()
{
    Q_ASSERT(drawn_textures.length() > 0);
    Q_ASSERT(current_layer < drawn_textures.length());
    Q_ASSERT(drawn_textures[current_layer].length() > 0);
    Q_ASSERT(current_frame < drawn_textures[current_layer].length());
    QImage &img = drawn_textures[current_layer][current_frame];

    QImage generated = gen_background(img.size(), tilesize);
    QPainter painter(&generated);
    painter.drawImage(0, 0, img);

    ui->textureViewLabel->setPixmap(QPixmap::fromImage(generated));
}
