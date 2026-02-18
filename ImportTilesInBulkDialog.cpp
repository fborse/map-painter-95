#include "ImportTilesInBulkDialog.hpp"
#include "ui_ImportTilesInBulkDialog.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QColorDialog>
#include <QPainter>

ImportTilesInBulkWidget::ImportTilesInBulkWidget(QWidget *parent):
    QWidget(parent),
    tilesize{0}, original_texture{}, rectangles{},
    zoom{1}, scaling{1}, color_key{}, magnetic{true},
    selected{-1}, displayed_texture{}
{
    setFixedSize(0, 0);
}

void ImportTilesInBulkWidget::removeRectangleAt(const int index)
{
    Q_ASSERT(0 <= index && index < rectangles.length());

    rectangles.remove(index);
    if (selected == index)
        selected = -1;
    update();
}

QRect &ImportTilesInBulkWidget::getRectangleAt(const int index)
{
    Q_ASSERT(0 <= index && index < rectangles.length());

    return rectangles[index];
}

//  ensure the correct image format
static inline QImage load_texture(const QString &path)
{
    QImage src(path);
    QImage dest(src.size(), QImage::Format_ARGB32_Premultiplied);
    dest.fill(Qt::transparent);

    QPainter painter(&dest);
    painter.drawImage(0, 0, src);

    return dest;
}

void ImportTilesInBulkWidget::setTexture(const QString &path)
{
    original_texture = load_texture(path);
    updateDisplayedTexture();
}

static inline void scale_texture(QImage &texture, const double factor)
{
    texture = texture.scaled(texture.size() * factor);
}

static inline void apply_color_key(QImage &texture, const QColor &ck)
{
    for (int j = 0; j < texture.height(); ++j)
        for (int i = 0; i < texture.width(); ++i)
            if (texture.pixelColor(i, j) == ck)
                texture.setPixelColor(i, j, Qt::transparent);
}

void ImportTilesInBulkWidget::updateDisplayedTexture()
{
    if (original_texture.isNull())
    {
        displayed_texture = {};
        setFixedSize(0, 0);
    }
    else
    {
    //  performances are probably not a bottleneck here
        displayed_texture = original_texture.copy();

        if (zoom * scaling >= 1)
        {
            apply_color_key(displayed_texture, color_key);
            scale_texture(displayed_texture, zoom * scaling);
        }
        else
        {
            scale_texture(displayed_texture, zoom * scaling);
            apply_color_key(displayed_texture, color_key);
        }

        setFixedSize(displayed_texture.size());
    }

    update();
}

//  tilesize * zoom at function call
static inline void draw_background(QPainter &painter, const QSize &paint_area, const int tilesize)
{
    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};
    const int s = tilesize;

    for (int j = 0; j < paint_area.height() / tilesize + 1; ++j)
    {
        for (int i = 0; i < paint_area.width() / tilesize + 1; ++i)
        {
            painter.fillRect(i*s, j*s, s/2, s/2, dark);
            painter.fillRect(i*s + s/2, j*s, qCeil(s/2.0), s/2, light);
            painter.fillRect(i*s, j*s + s/2, s/2, qCeil(s/2.0), light);
            painter.fillRect(i*s + s/2, j*s + s/2, qCeil(s/2.0), qCeil(s/2.0), dark);
        }
    }
}

static inline void draw_grid(QPainter &painter, const QSize &paint_area, const int tilesize)
{
    const QColor white128 = {255, 255, 255, 128};

    for (int j = 0; j < paint_area.height() / tilesize + 1; ++j)
        painter.fillRect(0, j * tilesize, paint_area.width(), 1, white128);
    for (int i = 0; i < paint_area.width() / tilesize + 1; ++i)
        painter.fillRect(i * tilesize, 0, 1, paint_area.height(), white128);
    for (int j = 0; j < paint_area.height() / tilesize + 1; ++j)
        painter.fillRect(0, (j+1) * tilesize - 1, paint_area.width(), 1, white128);
    for (int i = 0; i < paint_area.width() / tilesize + 1; ++i)
        painter.fillRect((i+1) * tilesize - 1, 0, 1, paint_area.height(), white128);
}

static inline void draw_points(QPainter &painter, const QSize &paint_area, const int tilesize)
{
    for (int j = 0; j < paint_area.height() / tilesize + 1; ++j)
        for (int i = 0; i < paint_area.width() / tilesize + 1; ++i)
            painter.fillRect(i * tilesize - 2, j * tilesize - 2, 4, 4, Qt::white);
}

void ImportTilesInBulkWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    draw_background(painter, size(), tilesize * zoom);
    painter.drawImage(0, 0, displayed_texture);
    draw_grid(painter, size(), tilesize * zoom);
    if (magnetic)
        draw_points(painter, size(), tilesize * zoom);
}

ImportTilesInBulkDialog::ImportTilesInBulkDialog(const int tilesize, QWidget *parent):
    QDialog(parent), ui(new Ui::ImportTilesInBulkDialog),
    tilesize{tilesize}
{
    ui->setupUi(this);

    ui->rectanglesViewWidget->setTilesize(tilesize);
    ui->colorKeyWidget->setColor(Qt::transparent);

    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 3);
}

ImportTilesInBulkDialog::~ImportTilesInBulkDialog()
{
    delete ui;
}

QString ImportTilesInBulkDialog::getTexturePath() const
{
    return ui->pathLineEdit->text();
}

QVector<QRect> ImportTilesInBulkDialog::getTileAreas() const
{
    return ui->rectanglesViewWidget->getRectangles();
}

void ImportTilesInBulkDialog::onChangeTexturePath()
{
    const char *title = "Choose a texture for the tiles to import !";
    const char *ext = "Texture files (*.bmp *.png *.jpg)";
    const QString path = QFileDialog::getOpenFileName(this, tr(title), QString(), tr(ext));

    if (!path.isEmpty())
    {
        ui->pathLineEdit->setText(path);
        ui->rectanglesViewWidget->setTexture(path);
    }
}

void ImportTilesInBulkDialog::onChangeColorKey()
{
    const QColor initial = ui->colorKeyWidget->getColor();
    const char *title = "Choose a color key !";
    const QColor ck = QColorDialog::getColor(initial, this, tr(title));
    if (ck.isValid())
    //  TODO: This step here is buggy
        ui->colorKeyWidget->setColor(ck.toRgb());
}

void ImportTilesInBulkDialog::onAccept() try
{
    if (getTexturePath().isEmpty())
        throw std::runtime_error("Cannot import tiles without texture !");
//    if (ui->rectanglesListWidget->)

    accept();
}
catch (std::runtime_error &error)
{
    const char *title = "Cannot import the tiles yet !";
    QMessageBox::information(this, tr(title), tr(error.what()));
}

void ImportTilesInBulkDialog::onAddArea()
{}

void ImportTilesInBulkDialog::onRemoveArea()
{}

void ImportTilesInBulkDialog::onSelectedAreaChanged(const int /*index*/)
{}
