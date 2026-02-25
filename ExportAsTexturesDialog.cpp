#include "ExportAsTexturesDialog.hpp"
#include "ui_ExportAsTexturesDialog.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QPainter>

ExportAsTexturesDialog::ExportAsTexturesDialog(const int tilesize, QWidget *parent):
    QDialog(parent), ui(new Ui::ExportAsTexturesDialog),
    tilesize{tilesize}, current_layer{0}, drawn_layers{},
    tileset{nullptr}, map_layers{nullptr}
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

//  TODO: the combinatorics for every used tile
int ExportAsTexturesDialog::getNumberOfFrames() const
{
    return 1;
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

    for (int l = 0; l < getNumberOfLayers(); ++l)
    {
        for (int f = 0; f < getNumberOfFrames(); ++f)
        {
            const QDir dir(getDirectory());
            QString pattern = getPattern();
            pattern.replace('#', QString::number(l + 1));
            pattern.replace('%', QString::number(f + 1));
            const QString path = dir.absoluteFilePath(pattern);

        //  TODO: provide a replace/discard/rename/cancel dialog
/*            if (dir.exists(pattern))
                throw QString("A file with that name exists already !");
            else*/ if (!drawn_layers[l].save(path))
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

static inline QImage gen_layer(const int tilesize, const Tileset &tileset, const MapLayer &layer)
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
            const auto id = layer[j][i];

            if (!id.isEmpty())
                painter.drawImage(i * tilesize, j * tilesize, tileset.value(layer[j][i]));
        }
    }

    return image;
}

void ExportAsTexturesDialog::redrawLayers()
{
    QSharedPointer<Tileset> tileset_ptr = tileset.toStrongRef();
    Q_ASSERT(!tileset_ptr.isNull());
    QSharedPointer<MapLayers> map_layers_ptr = map_layers.toStrongRef();
    Q_ASSERT(!map_layers_ptr.isNull());
    Q_ASSERT(current_layer < map_layers_ptr->length());

    ui->currentLayerComboBox->blockSignals(true);
    ui->currentLayerComboBox->clear();
    for (int k = 0; k < map_layers_ptr->length(); ++k)
    {
        drawn_layers.push_back(gen_layer(tilesize, *tileset_ptr, map_layers_ptr->at(k)));
        ui->currentLayerComboBox->addItem(QString::number(k + 1));
    }
    ui->currentLayerComboBox->blockSignals(false);

    updateLayerLabel();
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

void ExportAsTexturesDialog::updateLayerLabel()
{
    Q_ASSERT(current_layer < drawn_layers.length());
    QImage &img = drawn_layers[current_layer];

    QImage generated = gen_background(img.size(), tilesize);
    QPainter painter(&generated);
    painter.drawImage(0, 0, img);

    ui->textureViewLabel->setPixmap(QPixmap::fromImage(generated));
}
