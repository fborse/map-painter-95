#include "ImportSingleTileDialog.hpp"
#include "ui_ImportSingleTileDialog.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QColorDialog>
#include <QPainter>
#include <QMouseEvent>

ImportSingleTileWidget::ImportSingleTileWidget(QWidget *parent):
    QWidget(parent),
    tilesize{0}, original_texture{}, frames{},
    zoom{1}, scaling{1}, color_key{}, magnetic{true},
    selected{-1}, displayed_texture{},
    mouse_cursor{}, click_origin{}
{
    setFixedSize(0, 0);
}

void ImportSingleTileWidget::removeFrameAt(const int index)
{
    Q_ASSERT(0 <= index && index < frames.length());

    frames.remove(index);
    if (selected == index)
        selected = -1;
    update();
}

QPoint &ImportSingleTileWidget::getFrameAt(const int index)
{
    Q_ASSERT(0 <= index && index < frames.length());

    return frames[index];
}

SimpleTile ImportSingleTileWidget::getTile() const
{
    SimpleTile tile;

    for (auto &[x, y]: frames)
        tile.frames.push_back(
            displayed_texture.copy(QRect(x * scaling, y * scaling, tilesize, tilesize))
        );

    return tile;
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

void ImportSingleTileWidget::setTexture(const QString &path)
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
            if (texture.pixelColor(i, j).toHsv() == ck)
                texture.setPixelColor(i, j, Qt::transparent);
}

void ImportSingleTileWidget::updateDisplayedTexture()
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

void ImportSingleTileWidget::drawBackground(QPainter &painter)
{
    const int s = tilesize * zoom;

    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};

    for (int j = 0; j < height() / tilesize + 1; ++j)
    {
        for (int i = 0; i < width() / tilesize + 1; ++i)
        {
            painter.fillRect(i*s, j*s, s/2, s/2, dark);
            painter.fillRect(i*s + s/2, j*s, qCeil(s/2.0), s/2, light);
            painter.fillRect(i*s, j*s + s/2, s/2, qCeil(s/2.0), light);
            painter.fillRect(i*s + s/2, j*s + s/2, qCeil(s/2.0), qCeil(s/2.0), dark);
        }
    }
}

void ImportSingleTileWidget::drawFrames(QPainter &painter)
{
    const int unit = tilesize * zoom;

    const QColor white128 = {255, 255, 255, 128};
    const QColor white192 = {255, 255, 255, 192};

    for (int i = 0; i < frames.length(); ++i)
    {
        const QRect rect(frames[i] * zoom, QSize(unit, unit));

        painter.setPen((i == selected)? Qt::white : white192);
        painter.drawRect(rect);
        painter.fillRect(rect, (i == selected)? white192 : white128);
    }
}

//  we don't want QPoint's rounding
static inline QPoint divide(const QPoint &p, const double f)
{
    return {int(p.x() / f), int(p.y() / f)};
}

void ImportSingleTileWidget::drawFutureFrame(QPainter &painter)
{
    const int unit = tilesize * zoom;

    const QColor white192 = {255, 255, 255, 192};
    QPoint p = mouse_cursor;
    if (magnetic)
        p = divide(p, tilesize) * tilesize;
    const QRect rect(p * zoom, QSize(unit, unit));

    painter.setPen(Qt::white);
    painter.drawRect(rect);
    painter.fillRect(rect, white192);
}

void ImportSingleTileWidget::drawGrid(QPainter &painter)
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

void ImportSingleTileWidget::drawSnapPoints(QPainter &painter, const int unit)
{
    for (int j = 0; j < height() / unit + 1; ++j)
        for (int i = 0; i < width() / unit + 1; ++i)
            painter.fillRect(i * unit - 2, j * unit - 2, 4, 4, Qt::white);
}

void ImportSingleTileWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    drawBackground(painter);
    painter.drawImage(0, 0, displayed_texture);

    drawFrames(painter);
    if (click_origin && selected < 0)
        drawFutureFrame(painter);
    drawGrid(painter);

    if (magnetic)
        drawSnapPoints(painter, tilesize * zoom);
}

void ImportSingleTileWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouse_cursor = divide(event->pos(), zoom);

    if (move_offset)
    {
        QPoint new_position = mouse_cursor - *move_offset;
        if (magnetic)
        //  rounding is wanted here though
            new_position = (new_position / tilesize) * tilesize;

        emit frameChanged(new_position);
    }

    update();
}

static inline std::optional<int> rect_at(const QVector<QPoint> &frames, const QPoint &p, const int tilesize)
{
    for (int i = 0; i < frames.length(); ++i)
    {
        const QRect rect(frames[i], QSize(tilesize, tilesize));

        if (rect.contains(p))
            return i;
    }

    return {};
}

void ImportSingleTileWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        click_origin = mouse_cursor = divide(event->pos(), zoom);

        if (const auto idx = rect_at(frames, mouse_cursor, tilesize))
        {
            emit frameSelected(*idx);
            move_offset = mouse_cursor - frames[*idx];
        }
        else
        {
            emit frameSelected(-1);
        }
    }

    if (event->button() == Qt::RightButton)
        if (const auto idx = rect_at(frames, mouse_cursor, tilesize))
            emit frameRemoved(*idx);

    update();
}

void ImportSingleTileWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (selected < 0)
        {
            if (magnetic)
                emit frameAdded(divide(mouse_cursor, tilesize) * tilesize);
            else
                emit frameAdded(mouse_cursor);
        }

        click_origin = {};
        move_offset = {};
    }

    update();
}

ImportSingleTileDialog::ImportSingleTileDialog(const int tilesize, QWidget *parent):
    QDialog(parent), ui(new Ui::ImportSingleTileDialog),
    tilesize{tilesize}
{
    ui->setupUi(this);

    ui->tileView->setTilesize(tilesize);
    ui->colorKeyWidget->setColor(Qt::transparent);

    enableFramesWidgets(false);
    enableFrameWidgets(false);
}

ImportSingleTileDialog::~ImportSingleTileDialog()
{
    delete ui;
}

QString ImportSingleTileDialog::getTexturePath() const
{
    return ui->texturePathLineEdit->text();
}

QVector<QPoint> ImportSingleTileDialog::getFrames() const
{
    return ui->tileView->getFrames();
}

SimpleTile ImportSingleTileDialog::getTile() const
{
    return ui->tileView->getTile();
}

void ImportSingleTileDialog::onChangeTexturePath()
{
    const char *title = "Choose a texture for the tile to import !";
    const char *ext = "Texture files (*.bmp *.png *.jpg)";
    const QString path = QFileDialog::getOpenFileName(this, tr(title), QString(), tr(ext));

    if (!path.isEmpty())
    {
        enableFramesWidgets(true);
        ui->texturePathLineEdit->setText(path);
        ui->tileView->setTexture(path);
    }
}

void ImportSingleTileDialog::onChangeColorKey()
{
    const QColor initial = ui->colorKeyWidget->getColor();
    const char *title = "Choose a color key !";
    const QColor ck = QColorDialog::getColor(initial, this, tr(title));

    if (ck.isValid())
        ui->colorKeyWidget->setColor(ck.toRgb());
}

void ImportSingleTileDialog::onAccept() try
{
    if (getTexturePath().isEmpty())
        throw std::runtime_error("Cannot import a tile without texture !");
    if (getFrames().isEmpty())
        throw std::runtime_error("No frame found !");

    accept();
}
catch (std::runtime_error &error)
{
    const char *title = "Cannot import the tile yet !";
    QMessageBox::information(this, tr(title), tr(error.what()));
}

void ImportSingleTileDialog::enableFramesWidgets(const bool enabled)
{
    ui->scalingLabel->setEnabled(enabled);
    ui->scalingDoubleSpinBox->setEnabled(enabled);
    ui->colorKeyLabel->setEnabled(enabled);
    ui->colorKeyWidget->setEnabled(enabled);
    ui->zoomLabel->setEnabled(enabled);
    ui->zoomDoubleSpinBox->setEnabled(enabled);

    ui->framesListWidget->setEnabled(enabled);
    ui->addFramePushButton->setEnabled(enabled);

    ui->magneticCheckBox->setEnabled(enabled);
}

void ImportSingleTileDialog::onAddFrame(const QPoint frame)
{
    ui->tileView->addFrame(frame);
    const QString str = QString("(%1, %2)").arg(frame.x()).arg(frame.y());
    ui->framesListWidget->addItem(str);

    const int n = ui->framesListWidget->count();
    ui->framesListWidget->setCurrentRow(n - 1);
}

void ImportSingleTileDialog::onRemoveFrame(int index)
{
    if (index < 0)
        index = ui->framesListWidget->currentRow();
    Q_ASSERT(index >= 0);

    ui->framesListWidget->setCurrentRow(-1);
    delete ui->framesListWidget->takeItem(index);
    ui->tileView->removeFrameAt(index);
}

void ImportSingleTileDialog::enableFrameWidgets(const bool enabled)
{
    ui->removeFramePushButton->setEnabled(enabled);

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

void ImportSingleTileDialog::onSelectedFrameChanged(const int index)
{
    const auto &frames = ui->tileView->getFrames();
    Q_ASSERT(index < frames.length());

    enableFrameWidgets(index >= 0);

    ui->tileView->setSelectedFrame(index);
    if (index >= 0)
    {
        set_value(ui->xSpinBox, 0);
        set_value(ui->ySpinBox, 0);
    }
}

void ImportSingleTileDialog::onSelectFrame(const int index)
{
    ui->framesListWidget->setCurrentRow(index);
}

void ImportSingleTileDialog::onChangeFrame()
{
    const int index = ui->framesListWidget->currentRow();
    Q_ASSERT(0 <= index);

    QPoint &frame = ui->tileView->getFrameAt(index);
    frame.setX(ui->xSpinBox->value());
    frame.setY(ui->ySpinBox->value());

    ui->tileView->update();
}

void ImportSingleTileDialog::onFrameChanged(const QPoint new_position)
{
    ui->xSpinBox->setValue(new_position.x());
    ui->ySpinBox->setValue(new_position.y());
}
