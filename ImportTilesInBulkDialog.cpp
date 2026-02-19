#include "ImportTilesInBulkDialog.hpp"
#include "ui_ImportTilesInBulkDialog.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QColorDialog>
#include <QPainter>
#include <QMouseEvent>

ImportTilesInBulkWidget::ImportTilesInBulkWidget(QWidget *parent):
    QWidget(parent),
    tilesize{0}, original_texture{}, rectangles{},
    zoom{1}, scaling{1}, color_key{}, magnetic{true},
    selected{-1}, displayed_texture{},
    mouse_cursor{}, click_origin{}
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

void ImportTilesInBulkWidget::drawBackground(QPainter &painter)
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

void ImportTilesInBulkWidget::drawRectangles(QPainter &painter)
{
    const int unit = tilesize * zoom;

    const QColor white128 = {255, 255, 255, 128};
    const QColor white192 = {255, 255, 255, 192};

    for (int i = 0; i < rectangles.length(); ++i)
    {
        const QPoint position = rectangles[i].topLeft();
        const QSize aspect = rectangles[i].size();
        const QRect rect = {position * zoom, aspect * unit};

        painter.setPen((i == selected)? Qt::white : white192);
        painter.drawRect(rect);
        painter.fillRect(rect, (i == selected)? white192 : white128);
    }
}

void ImportTilesInBulkWidget::drawGrid(QPainter &painter)
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

void ImportTilesInBulkWidget::drawSnapPoints(QPainter &painter, const int unit)
{
    for (int j = 0; j < height() / unit + 1; ++j)
        for (int i = 0; i < width() / unit + 1; ++i)
            painter.fillRect(i * unit - 2, j * unit - 2, 4, 4, Qt::white);
}

static inline QPoint get_mins(const QPoint &p1, const QPoint &p2)
{
    return QPoint(qMin(p1.x(), p2.x()), qMin(p1.y(), p2.y()));
}

static inline QPoint get_maxs(const QPoint &p1, const QPoint &p2)
{
    return QPoint(qMax(p1.x(), p2.x()), qMax(p1.y(), p2.y()));
}

QRect ImportTilesInBulkWidget::getSelectionRect() const
{
    QPoint top_left = get_mins(*click_origin, mouse_cursor);
    QPoint bottom_right = get_maxs(*click_origin, mouse_cursor);

    const int unit = tilesize * zoom;

    if (magnetic)
    //  this time we want the QPoint rounding
        return QRect((top_left / unit) * unit, (bottom_right / unit) * unit);
    else
        return QRect(top_left, bottom_right);
}

void ImportTilesInBulkWidget::drawSelectionRect(QPainter &painter)
{
    QRect rect = getSelectionRect();
//  QRect(QPoint, QPoint) adds 1 to each size components
    rect.setSize(rect.size() - QSize(1, 1));

    painter.setPen(Qt::white);
    painter.drawRect(rect);
    const QColor white192 = {255, 255, 255, 192};
    painter.fillRect(rect, white192);
}

void ImportTilesInBulkWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    drawBackground(painter);
    painter.drawImage(0, 0, displayed_texture);
    drawRectangles(painter);
    drawGrid(painter);
    if (magnetic)
        drawSnapPoints(painter, tilesize * zoom);
    if (click_origin && selected < 0)
        drawSelectionRect(painter);
}

//  we don't want QPoint's rounding
static inline QPoint divide(const QPoint &p, const double f)
{
    return {int(p.x() / f), int(p.y() / f)};
}

void ImportTilesInBulkWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouse_cursor = divide(event->pos(), zoom);
    if (move_offset)
    {
        QPoint new_position = mouse_cursor - *move_offset;
        if (magnetic)
            new_position = (new_position / tilesize) * tilesize;
        emit rectangleChanged(new_position);
    }

    update();
}

static inline std::optional<int> rect_at(const QVector<QRect> &rectangles, const QPoint &p, const int tilesize, const double zoom)
{
    const int unit = tilesize * zoom;

    for (int i = 0; i < rectangles.length(); ++i)
    {
        const QPoint position = rectangles[i].topLeft();
        const QSize aspect = rectangles[i].size();

        if (QRect(position * zoom, aspect * unit).contains(p))
            return i;
    }

    return {};
}

void ImportTilesInBulkWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        click_origin = mouse_cursor = divide(event->pos(), zoom);

        if (const auto idx = rect_at(rectangles, mouse_cursor, tilesize, zoom))
        {
            emit rectangleSelected(*idx);
            move_offset = mouse_cursor - rectangles[*idx].topLeft();
        }
        else
        {
            emit rectangleSelected(-1);
        }
    }

    update();
}

static inline QRect to_rect(const QPoint &p1, const QPoint &p2)
{
    return {
        qMin(p1.x(), p2.x()), qMin(p1.y(), p2.y()),
        qAbs(p1.x() - p2.x()), qAbs(p1.y() - p2.y())
    };
}

void ImportTilesInBulkWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (selected < 0)
        {
            QRect rect = getSelectionRect();
        //  QRect(QPoint, QPoint) adds 1 to each size components
            rect.setSize(rect.size() - QSize(1, 1));

            const auto &[x, y] = rect.topLeft();
        //  we want a truncating behaviour
            const auto &[w, h] = rect.size();

            emit rectangleAdded(QRect(x, y, w / tilesize, h / tilesize));
        }

        click_origin = {};
        move_offset = {};
    }

    update();
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

    enableAreasWidgets(false);
    enableAreaWidgets(false);
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
        enableAreasWidgets(true);
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

void ImportTilesInBulkDialog::enableAreasWidgets(const bool enabled)
{
    ui->scalingLabel->setEnabled(enabled);
    ui->scalingDoubleSpinBox->setEnabled(enabled);
    ui->colorKeyLabel->setEnabled(enabled);
    ui->colorKeyWidget->setEnabled(enabled);
    ui->zoomLabel->setEnabled(enabled);
    ui->zoomDoubleSpinBox->setEnabled(enabled);

    ui->rectanglesListWidget->setEnabled(enabled);
    ui->addPushButton->setEnabled(enabled);

    ui->magneticCheckBox->setEnabled(enabled);
}

void ImportTilesInBulkDialog::onAddArea(const QRect area)
{
    const QString rect = QString("(%1, %2) %3x%4")
        .arg(area.x()).arg(area.y())
        .arg(area.width()).arg(area.height());
    ui->rectanglesViewWidget->addRectangle(area);
    ui->rectanglesListWidget->addItem(rect);

    const int n = ui->rectanglesListWidget->count();
    ui->rectanglesListWidget->setCurrentRow(n - 1);
}

void ImportTilesInBulkDialog::onRemoveArea(int index)
{
    if (index < 0)
        index = ui->rectanglesListWidget->currentRow();
    Q_ASSERT(index >= 0);

    ui->rectanglesListWidget->setCurrentRow(-1);
    delete ui->rectanglesListWidget->takeItem(index);
    ui->rectanglesViewWidget->removeRectangleAt(index);
}

void ImportTilesInBulkDialog::enableAreaWidgets(const bool enabled)
{
    ui->removePushButton->setEnabled(enabled);

    ui->xLabel->setEnabled(enabled);
    ui->xSpinBox->setEnabled(enabled);
    ui->yLabel->setEnabled(enabled);
    ui->ySpinBox->setEnabled(enabled);
    ui->wLabel->setEnabled(enabled);
    ui->wSpinBox->setEnabled(enabled);
    ui->hLabel->setEnabled(enabled);
    ui->hSpinBox->setEnabled(enabled);
}

static inline void set_value(QSpinBox *widget, const int value)
{
    widget->blockSignals(true);
    widget->setValue(value);
    widget->blockSignals(false);
}

void ImportTilesInBulkDialog::onSelectedAreaChanged(const int index)
{
    const auto &areas = ui->rectanglesViewWidget->getRectangles();
    Q_ASSERT(index < areas.length());

    enableAreaWidgets(index >= 0);

    ui->rectanglesViewWidget->setSelectedRectangle(index);
    if (index >= 0)
    {
        set_value(ui->xSpinBox, areas[index].left());
        set_value(ui->ySpinBox, areas[index].top());
        set_value(ui->wSpinBox, areas[index].width());
        set_value(ui->hSpinBox, areas[index].height());
    }
}

void ImportTilesInBulkDialog::onSelectArea(const int index)
{
    ui->rectanglesListWidget->setCurrentRow(index);
}

void ImportTilesInBulkDialog::onChangeArea()
{
    const int index = ui->rectanglesListWidget->currentRow();
    Q_ASSERT(0 <= index);

    QRect &area = ui->rectanglesViewWidget->getRectangleAt(index);
    area.setX(ui->xSpinBox->value());
    area.setY(ui->ySpinBox->value());
    area.setWidth(ui->wSpinBox->value());
    area.setHeight(ui->hSpinBox->value());

    ui->rectanglesViewWidget->update();
}

void ImportTilesInBulkDialog::onAreaChanged(const QPoint new_position)
{
    ui->xSpinBox->setValue(new_position.x());
    ui->ySpinBox->setValue(new_position.y());
}
