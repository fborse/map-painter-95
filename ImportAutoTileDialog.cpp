#include "ImportAutoTileDialog.hpp"
#include "ui_ImportAutoTileDialog.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QColorDialog>
#include <QPainter>
#include <QMouseEvent>

AutoTileViewWidget::AutoTileViewWidget(QWidget *parent):
    QWidget(parent), tilesize{0}, metatiles(20), selected{-1}
{}

void AutoTileViewWidget::setMetatileImage(const int index, const QImage &image)
{
    Q_ASSERT(0 <= index && index < metatiles.length());
    metatiles[index] = image;
}

void AutoTileViewWidget::drawBackground(QPainter &painter)
{
    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};

    const int s = tilesize / 2;

    for (int j = 0; j < height() / s + 1; ++j)
    {
        for (int i = 0; i < width() / s + 1; ++i)
        {
            painter.fillRect(i * s, j * s, s/2, s/2, dark);
            painter.fillRect(i * s + s/2, j * s, s/2, s/2, light);
            painter.fillRect(i * s, j * s + s/2, s/2, s/2, light);
            painter.fillRect(i * s + s/2, j * s + s/2, s/2, s/2, dark);
        }
    }
}

void AutoTileViewWidget::drawMetatiles(QPainter &painter)
{
//  see Types.hpp for why 20
    Q_ASSERT(metatiles.length() == 20);

    const int s = tilesize / 2;
//  RPG Maker scheme
//  start with an isolated tile on the top left
    painter.drawImage(0, 0, metatiles[0]);
    painter.drawImage(s, 0, metatiles[3]);
    painter.drawImage(0, s, metatiles[12]);
    painter.drawImage(s, s, metatiles[15]);
//  then the joint tile on the top right
    painter.drawImage(2*s, 0, metatiles[16]);
    painter.drawImage(3*s, 0, metatiles[17]);
    painter.drawImage(2*s, s, metatiles[18]);
    painter.drawImage(3*s, s, metatiles[19]);
//  now the main part ; see Types.hpp for why 16
    for (int i = 0; i < 16; ++i)
    {
        const QPoint p(i % 4, 2 + i / 4);
        painter.drawImage(p * s, metatiles[i]);
    }
}

void AutoTileViewWidget::drawGrid(QPainter &painter)
{
    const int s = tilesize / 2;

//  RPG Maker scheme ; w = 4, h = 2 + 4
    for (int j = 0; j < 6; ++j)
        painter.fillRect(0, j * s, width(), 1, Qt::white);
    for (int i = 0; i < 4; ++i)
        painter.fillRect(i * s, 0, 1, height(), Qt::white);
    for (int j = 0; j < 6; ++j)
        painter.fillRect(0, j * s + s-1, width(), 1, Qt::white);
    for (int i = 0; i < 4; ++i)
        painter.fillRect(i * s + s-1, 0, 1, height(), Qt::white);
}

static inline void draw_rect(QPainter &painter, const QPoint p, const QSize s)
{
    painter.setPen(Qt::black);
    painter.drawRect(QRect(p, s));

    painter.setPen(Qt::white);
    painter.drawRect(QRect(p + QPoint(1, 1), s - QSize(2, 2)));
}

void AutoTileViewWidget::drawSelectionRect(QPainter &painter)
{
    if (selected >= 0)
    {
        QPoint p(selected % 4, 2 + selected / 4);
    //  joints are displayed on the top right
        if (selected == 16)
            p = {2, 0};
        else if (selected == 17)
            p = {3, 0};
        else if (selected == 18)
            p = {2, 1};
        else if (selected == 19)
            p = {2, 2};

        const QSize s(tilesize/2, tilesize/2);
        draw_rect(painter, p * (tilesize/2), s);

    //  let's also display another cursor on the single tile if relevant
        if (selected == 0)
            draw_rect(painter, {0, 0}, s);
        else if (selected == 3)
            draw_rect(painter, {tilesize/2, 0}, s);
        else if (selected == 12)
            draw_rect(painter, {0, tilesize/2}, s);
        else if (selected == 15)
            draw_rect(painter, {tilesize/2, tilesize/2}, s);
    }
}

void AutoTileViewWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    drawBackground(painter);
    drawMetatiles(painter);
    drawGrid(painter);
    drawSelectionRect(painter);
}

//  QPoint's division operator rounds instead of truncating
static inline QPoint divide(const QPoint &p, const double f)
{
    return {int(p.x() / f), int(p.y() / f)};
}

void AutoTileViewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        const auto p = divide(event->pos(), tilesize/2);

    //  RPG Maker scheme ; two first rows are special
        if (p.y() < 2)
        {
        //  single tile
            if (p == QPoint(0, 0))
                emit metatileSelected(0);
            else if (p == QPoint(1, 0))
                emit metatileSelected(3);
            else if (p == QPoint(0, 1))
                emit metatileSelected(12);
            else if (p == QPoint(1,1))
                emit metatileSelected(15);
        //  joints
            if (p == QPoint(2, 0))
                emit metatileSelected(16);
            else if (p == QPoint(3, 0))
                emit metatileSelected(17);
            else if (p == QPoint(2, 1))
                emit metatileSelected(18);
            else if (p == QPoint(3, 1))
                emit metatileSelected(19);
        }
        else
        {
        //  the widget has 4 columns
            emit metatileSelected(p.x() + (p.y() - 2) * 4);
        }
    }
}

ImportAutoTileWidget::ImportAutoTileWidget(QWidget *parent):
    QWidget(parent),
    tilesize{0}, original_texture{}, metatiles(20),
    zoom{1}, scaling{1}, color_key{}, magnetic{true},
    selected{-1}, displayed_texture{},
    mouse_cursor{}, click_origin{}
{
    setFixedSize(0, 0);
}

QPoint &ImportAutoTileWidget::getMetatileAt(const int index)
{
    Q_ASSERT(0 <= index && index < metatiles.length());

    return metatiles[index];
}

AutoTile ImportAutoTileWidget::getAutoTile() const
{
    const int s = tilesize / 2;

    AutoTile::Frame frame;
    for (auto &[x, y]: metatiles)
        frame.metatiles.push_back(displayed_texture.copy(x, y, s, s));

    return {{frame}};
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

void ImportAutoTileWidget::setTexture(const QString &path)
{
    original_texture = load_texture(path);
    updateDisplayedTexture();
}

void ImportAutoTileWidget::setTilesize(const int size)
{
    tilesize = size;

//  Modified RPG Maker scheme
    for (int i = 0; i < metatiles.length(); ++i)
        metatiles[i] = QPoint(i % 4, i / 4) * (tilesize / 2);

    update();
}

static inline void scale_texture(QImage &texture, const double factor)
{
    texture = texture.scaled(texture.size() * factor);
}

static inline void apply_color_key(QImage &texture, const QColor &ck)
{
    for (int j = 0; j < texture.height(); ++j)
        for (int i = 0; i < texture.width(); ++i)
            if (texture.pixelColor(i, j).toHsv() == ck)
                texture.setPixelColor(i, j, Qt::transparent);
}

void ImportAutoTileWidget::updateDisplayedTexture()
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

        if (scaling >= 1)
        {
            apply_color_key(displayed_texture, color_key);
            scale_texture(displayed_texture, zoom * scaling);
        }
        else
        {
            scale_texture(displayed_texture, zoom * scaling);
            apply_color_key(displayed_texture, color_key);
        }

        setFixedSize(displayed_texture.size() * zoom);
        emit displayedTextureChanged();
    }

    update();
}

void ImportAutoTileWidget::drawBackground(QPainter &painter)
{
    const int s = tilesize * zoom;

    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};

    for (int j = 0; j < height() / tilesize + 1; ++j)
    {
        for (int i = 0; i < width() / tilesize + 1; ++i)
        {
            painter.fillRect(i*s, j*s, s/2, s/2, dark);
            painter.fillRect(i*s + s/2, j*s, s/2, s/2, light);
            painter.fillRect(i*s, j*s + s/2, s/2, s/2, light);
            painter.fillRect(i*s + s/2, j*s + s/2, s/2, s/2, dark);
        }
    }
}

void ImportAutoTileWidget::drawMetatiles(QPainter &painter)
{
    const int unit = tilesize/2 * zoom;

    const QColor white128 = {255, 255, 255, 128};
    const QColor white192 = {255, 255, 255, 192};

    for (int i = 0; i < metatiles.length(); ++i)
    {
        const QRect rect(metatiles[i] * zoom, QSize(unit, unit));

        painter.setPen((i == selected)? Qt::white : white192);
        painter.drawRect(rect);
        painter.fillRect(rect, (i == selected)? white192 : white128);
    }
}

void ImportAutoTileWidget::drawGrid(QPainter &painter)
{
    const int unit = tilesize * zoom;

    const QColor white128 = {255, 255, 255, 128};

    for (int j = 0; j < height() / unit + 1; ++j)
        painter.fillRect(0, j * unit, width(), 1, white128);
    for (int i = 0; i < width() / unit + 1; ++i)
        painter.fillRect(i * unit, 0, 1, height(), white128);
    for (int j = 0; j < height() / unit + 1; ++j)
        painter.fillRect(0, (j+1) * unit - 1, width(), 1, white128);
    for (int i = 0; i < width() / unit + 1; ++i)
        painter.fillRect((i+1) * unit - 1, 0, 1, height(), white128);
}

void ImportAutoTileWidget::drawSnapPoints(QPainter &painter, const int unit)
{
    for (int j = 0; j < height() / unit + 1; ++j)
        for (int i = 0; i < width() / unit + 1; ++i)
            painter.fillRect(i * unit - 2, j * unit - 2, 4, 4, Qt::white);
}

void ImportAutoTileWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    drawBackground(painter);
    painter.scale(zoom, zoom);
    painter.drawImage(0, 0, displayed_texture);
    painter.resetTransform();

    drawMetatiles(painter);
    drawGrid(painter);

    if (magnetic)
        drawSnapPoints(painter, tilesize/2 * zoom);
}

void ImportAutoTileWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouse_cursor = divide(event->pos(), zoom);

    if (click_origin)
    {
        Q_ASSERT(selected >= 0);

        QPoint new_position = mouse_cursor - *click_origin;
        if (magnetic)
        //  rounding is wanted here
            new_position = (new_position / (tilesize/2)) * (tilesize/2);

        emit metatileChanged(new_position);
    }

    update();
}

static inline std::optional<int> rect_at(const QVector<QPoint> &metatiles, const QPoint &p, const int unit)
{
    for (int i = 0; i < metatiles.length(); ++i)
    {
        const QRect rect(metatiles[i], QSize(unit, unit));

        if (rect.contains(p))
            return i;
    }

    return {};
}

void ImportAutoTileWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        mouse_cursor = divide(event->pos(), zoom);

        if (const auto idx = rect_at(metatiles, mouse_cursor, tilesize/2))
        {
            emit metatileSelected(*idx);
            click_origin = mouse_cursor - metatiles[*idx];
        }
        else
        {
            emit metatileSelected(-1);
        }
    }

    update();
}

void ImportAutoTileWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        click_origin = {};

    update();
}

ImportAutoTileDialog::ImportAutoTileDialog(const int tilesize, QWidget *parent):
    QDialog(parent), ui(new Ui::ImportAutoTileDialog),
    tilesize{tilesize}
{
    ui->setupUi(this);

    ui->metatilesView->setTilesize(tilesize);
    ui->autotileView->setTilesize(tilesize);
    ui->colorKeyWidget->setColor(Qt::transparent);

    enableMetatilesWidgets(false);
    enableMetatileWidgets(false);
}

ImportAutoTileDialog::~ImportAutoTileDialog()
{
    delete ui;
}

QString ImportAutoTileDialog::getTexturePath() const
{
    return ui->pathLineEdit->text();
}

AutoTile ImportAutoTileDialog::getAutoTile() const
{
    return ui->autotileView->getAutoTile();
}

void ImportAutoTileDialog::onChangeTexturePath()
{
    const char *title = "Choose a texture for the autotile to import !";
    const char *ext = "Texture files (*.bmp *.png *.jpg)";
    const QString path = QFileDialog::getOpenFileName(this, tr(title), QString(), tr(ext));

    if (!path.isEmpty())
    {
        enableMetatilesWidgets(true);
        ui->pathLineEdit->setText(path);

        QImage texture(path);
    //  see Types.hpp for why 20
        for (int i = 0; i < 20; ++i)
            ui->metatilesView->setMetatileImage(i, texture.copy(0, 0, tilesize/2, tilesize/2));

        ui->autotileView->setTexture(path);
    }
}

void ImportAutoTileDialog::onChangeColorKey()
{
    const QColor initial = ui->colorKeyWidget->getColor();
    const char *title = "Choose a color key !";
    const QColor ck = QColorDialog::getColor(initial, this, tr(title));

    if (ck.isValid())
        ui->colorKeyWidget->setColor(ck.toRgb());
}

void ImportAutoTileDialog::onAccept() try
{
    if (getTexturePath().isEmpty())
        throw std::runtime_error("Cannot import an autotile without texture !");

    accept();
}
catch (std::runtime_error &error)
{
    const char *title = "Cannot import the autotile yet !";
    QMessageBox::information(this, tr(title), tr(error.what()));
}

void ImportAutoTileDialog::enableMetatilesWidgets(const bool enabled)
{
    ui->scalingLabel->setEnabled(enabled);
    ui->scalingDoubleSpinBox->setEnabled(enabled);
    ui->colorKeyLabel->setEnabled(enabled);
    ui->colorKeyWidget->setEnabled(enabled);
    ui->zoomLabel->setEnabled(enabled);
    ui->zoomDoubleSpinBox->setEnabled(enabled);

    ui->metatilesView->setEnabled(enabled);
    ui->magneticCheckBox->setEnabled(enabled);
}

void ImportAutoTileDialog::enableMetatileWidgets(const bool enabled)
{
    ui->xLabel->setEnabled(enabled);
    ui->xSpinBox->setEnabled(enabled);
    ui->yLabel->setEnabled(enabled);
    ui->ySpinBox->setEnabled(enabled);
}

static inline void set_value(QSpinBox *widget, const int value)
{
    Q_ASSERT(widget != nullptr);

    widget->blockSignals(true);
    widget->setValue(value);
    widget->blockSignals(false);
}

void ImportAutoTileDialog::onSelectedMetatileChanged(const int index)
{
//  see Types.hpp for why 20
    Q_ASSERT(index < 20);

    enableMetatileWidgets((index >= 0));

    ui->autotileView->setSelectedMetatile(index);
    if (index >= 0)
    {
        set_value(ui->xSpinBox, 0);
        set_value(ui->ySpinBox, 0);
    }
}

void ImportAutoTileDialog::onChangeMetatile()
{
    const int index = ui->metatilesView->getSelectedMetatile();
    Q_ASSERT(0 <= index);

    QPoint &p = ui->autotileView->getMetatileAt(index);
    p.setX(ui->xSpinBox->value());
    p.setY(ui->ySpinBox->value());

    ui->autotileView->update();

    const QSize s = {tilesize/2, tilesize/2};
    const auto &img = ui->autotileView->getDisplayedTexture();
    ui->metatilesView->setMetatileImage(index, img.copy(QRect(p, s)));
    ui->metatilesView->update();
}

void ImportAutoTileDialog::onMetatileChanged(const QPoint new_position)
{
    ui->xSpinBox->setValue(new_position.x());
    ui->ySpinBox->setValue(new_position.y());
}

void ImportAutoTileDialog::redrawMetatiles()
{
    const QSize s = {tilesize/2, tilesize/2};
    const auto &img = ui->autotileView->getDisplayedTexture();

    const auto p = ui->autotileView->getMetatiles();
    for (int i = 0; i < 20; ++i)
        ui->metatilesView->setMetatileImage(i, img.copy(QRect(p[i], s)));

    ui->metatilesView->update();
}
