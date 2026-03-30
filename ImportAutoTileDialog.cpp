#include "ImportAutoTileDialog.hpp"
#include "ui_ImportAutoTileDialog.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QColorDialog>
#include <QPainter>
#include <QMouseEvent>

AutoTileViewWidget::AutoTileViewWidget(QWidget *parent):
//  see Types.hpp for why 20
    QWidget(parent), tilesize{0}, metatiles(20), selections(20, false)
{}

void AutoTileViewWidget::setMetatileState(const int index, const bool state)
{
    Q_ASSERT(0 <= index && index < metatiles.length());
    selections[index] = state;

    emit selectionChanged();
    update();
}

void AutoTileViewWidget::setMetatileImage(const int index, const QImage &image)
{
    Q_ASSERT(0 <= index && index < metatiles.length());
    metatiles[index] = image;
    update();
}

void AutoTileViewWidget::clearSelections()
{
    for (auto &metatile: selections)
        metatile = false;

    emit selectionChanged();
    update();
}

QSet<int> AutoTileViewWidget::getSelectedMetatiles() const
{
    QSet<int> selected;

    for (int i = 0; i < selections.length(); ++i)
        if (selections[i])
            selected.insert(i);

    return selected;
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

    if (!metatiles[0].isNull())
    {
    //  RPG Maker scheme
    //  start with an isolated tile on the top left
        const std::array<int, 4> isolated = {0, 3, 12, 15};
        for (size_t i = 0; i < isolated.size(); ++i)
            painter.drawImage(QPoint(i % 2, i / 2) * s, metatiles[isolated[i]].scaled({s, s}));
    //  then the joint tile on the top right
        const std::array<int, 4> joints = {16, 17, 18, 19};
        for (size_t i = 0; i < joints.size(); ++i)
            painter.drawImage(QPoint(2 + (i % 2), i / 2) * s, metatiles[joints[i]].scaled({s, s}));
    //  now the main part ; see Types.hpp for why 16
        for (int i = 0; i < 16; ++i)
            painter.drawImage(QPoint(i % 4, 2 + (i / 4)) * s, metatiles[i].scaled({s, s}));
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
    const QColor white128 = {255, 255, 255, 128};
    painter.fillRect(QRect(p, s), white128);

    painter.setPen(Qt::black);
    painter.drawRect(QRect(p, s));

    painter.setPen(Qt::white);
    painter.drawRect(QRect(p + QPoint(1, 1), s - QSize(2, 2)));
}

void AutoTileViewWidget::drawSelectionRects(QPainter &painter)
{
    const QSize s = {tilesize / 2, tilesize / 2};

//  RPG Maker scheme
//  top-left 2x2 patch is a single isolated tile
    const std::array<int, 4> isolated = {0, 3, 12, 15};
    for (size_t i = 0; i < isolated.size(); ++i)
        if (selections[isolated[i]])
            draw_rect(painter, QPoint(i % 2, i / 2) * tilesize/2, s);

//  top-right 2x2 patch are the joints
    const std::array<int, 4> joints = {16, 17, 18, 19};
    for (size_t i = 0; i < joints.size(); ++i)
        if (selections[16 + i])
            draw_rect(painter, QPoint(2 + (i % 2), i / 2) * tilesize/2, s);

//  bottom patch
    for (int i = 0; i < 16; ++i)
        if (selections[i])
            draw_rect(painter, QPoint(i % 4, 2 + (i / 4)) * tilesize/2, s);
}

void AutoTileViewWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    drawBackground(painter);
    drawMetatiles(painter);
    drawGrid(painter);
    drawSelectionRects(painter);
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

        int index = -1;
    //  RPG Maker scheme ; two first rows are special
        if (p.y() < 2)
        {
        //  single tile
            if (p == QPoint(0, 0))
                index = 0;
            else if (p == QPoint(1, 0))
                index = 3;
            else if (p == QPoint(0, 1))
                index = 12;
            else if (p == QPoint(1,1))
                index = 15;
        //  joints
            if (p == QPoint(2, 0))
                index = 16;
            else if (p == QPoint(3, 0))
                index = 17;
            else if (p == QPoint(2, 1))
                index = 18;
            else if (p == QPoint(3, 1))
                index = 19;
        }
        else
        {
        //  the widget has 4 columns
            index = p.x() + (p.y() - 2) * 4;
        }

        Q_ASSERT(0 <= index && index < 20);
        selections[index] = !selections[index];
        emit selectionChanged();

        update();
    }
}

ImportAutoTileWidget::ImportAutoTileWidget(QWidget *parent):
    QWidget(parent),
    tilesize{0}, original_texture{}, metatiles(20),
    zoom{1}, scaling{1}, color_key{}, magnetic{true},
    selected{}, displayed_texture{},
    mouse_cursor{}, rect_origin{}, move_origins{}
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

void ImportAutoTileWidget::setMetatilePosition(const int index, const QPoint new_position)
{
//  see Types.hpp for why 20
    Q_ASSERT(0 <= index && index < 20);
    metatiles[index] = new_position;
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
            scale_texture(displayed_texture, scaling);
        }
        else
        {
            scale_texture(displayed_texture, scaling);
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

void ImportAutoTileWidget::drawMetatileRects(QPainter &painter)
{
    const int unit = tilesize/2 * zoom;

    const QColor white128 = {255, 255, 255, 128};
    const QColor white192 = {255, 255, 255, 192};

    for (int i = 0; i < metatiles.length(); ++i)
    {
        const QRect rect(metatiles[i] * zoom, QSize(unit, unit));

        painter.setPen(selected.contains(i)? Qt::white : white192);
        painter.drawRect(rect);
        painter.fillRect(rect, selected.contains(i)? white192 : white128);
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

static inline QRect to_rect(const QPoint &p1, const QPoint &p2)
{
    return {
        qMin(p1.x(), p2.x()), qMin(p1.y(), p2.y()),
        qAbs(p1.x() - p2.x()), qAbs(p1.y() - p2.y())
    };
}

void ImportAutoTileWidget::drawSelectionOutline(QPainter &painter)
{
    if (rect_origin)
    {
        painter.setPen(Qt::black);
        painter.drawRect(to_rect(*rect_origin, mouse_cursor));

        const QPoint p = {1, 1};
        painter.setPen(Qt::white);
        painter.drawRect(to_rect(*rect_origin + p, mouse_cursor - p));
    }
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

    drawMetatileRects(painter);
    drawGrid(painter);
    drawSelectionOutline(painter);

    if (magnetic)
        drawSnapPoints(painter, tilesize/2 * zoom);
}

void ImportAutoTileWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouse_cursor = divide(event->pos(), zoom);

    if (!move_origins.isEmpty())
    {
        for (auto &i: selected)
        {
            QPoint new_position = mouse_cursor - move_origins[i];
            if (magnetic)
            //  rounding is wanted here
                new_position = (new_position / (tilesize/2)) * (tilesize/2);

            emit metatilePositionChanged(i, new_position);
        }
    }

    update();
}

static inline QSet<int> rects_at(const QVector<QPoint> &metatiles, const QPoint &p, const int unit)
{
    QSet<int> found;

    const QRect r = {{}, QSize(unit, unit)};
    for (int i = 0; i < metatiles.length(); ++i)
        if (r.translated(metatiles[i]).contains(p))
            found.insert(i);

    return found;
}

void ImportAutoTileWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        mouse_cursor = divide(event->pos(), zoom);

        const QSet<int> found = rects_at(metatiles, mouse_cursor, tilesize/2);
        if (!found.isEmpty())
        {
            if (selected.intersects(found))
            {
                move_origins.clear();
                for (auto &index: selected)
                    move_origins[index] = mouse_cursor - metatiles[index];
            }
            else
            {
                move_origins.clear();
                emit clearSelection();
                for (auto &index: found)
                {
                    move_origins[index] = mouse_cursor - metatiles[index];
                    emit metatileClicked(index);
                }
            }
        }
        else
        {
            rect_origin = mouse_cursor;
            emit clearSelection();
        }
    }

    update();
}

void ImportAutoTileWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (rect_origin)
        {
            const QRect r = to_rect(*rect_origin, mouse_cursor);

            emit clearSelection();
            for (int i = 0; i < metatiles.length(); ++i)
                if (r.contains(QRect(metatiles[i], QSize(tilesize/2, tilesize/2))))
                    emit metatileClicked(i);
        }

        rect_origin = {};
        move_origins.clear();
    }

    update();
}

ImportAutoTileDialog::ImportAutoTileDialog(const int tilesize, QWidget *parent):
    QDialog(parent), ui(new Ui::ImportAutoTileDialog),
    tilesize{tilesize}
{
    ui->setupUi(this);

    ui->metatilesView->setTilesize((tilesize/2 < 32)? 2*tilesize : tilesize);
    ui->autotileView->setTilesize(tilesize);
    ui->colorKeyWidget->setColor(Qt::transparent);

//  currying setMetatilestate since Qt Designer doesn't have a handy tool for that
    connect(ui->autotileView, &ImportAutoTileWidget::metatileClicked, [&] (const int index) {
        ui->metatilesView->setMetatileState(index, true);
    });
//  same here
    connect(ui->autotileView, &ImportAutoTileWidget::metatilePositionChanged, [&] (const int, const QPoint new_position) {
        onMetatileChanged(new_position);
    });

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
            ui->metatilesView->setMetatileImage(i, texture.copy(0, 0, tilesize, tilesize));

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

void ImportAutoTileDialog::onSelectedMetatilesChanged()
{
    const QSet<int> selected = ui->metatilesView->getSelectedMetatiles();
    enableMetatileWidgets(selected.count() == 1);

    if (selected.count() == 1)
    {
        const int index = *selected.cbegin();
    //  see Types.hpp for why 20
        Q_ASSERT(0 <= index && index < 20);

        const auto &[x, y] = ui->autotileView->getMetatileAt(index);
        set_value(ui->xSpinBox, x);
        set_value(ui->ySpinBox, y);
    }

    ui->autotileView->setSelectedMetatiles(selected);
}

void ImportAutoTileDialog::onMetatilePositionChanged(const int index, const QPoint new_position)
{
    const auto &img = ui->autotileView->getDisplayedTexture();

    const QRect r = {new_position, QSize(tilesize/2, tilesize/2)};
    ui->metatilesView->setMetatileImage(index, img.copy(r));
}

void ImportAutoTileDialog::onChangeMetatile()
{
    const QSet<int> selected = ui->metatilesView->getSelectedMetatiles();
    Q_ASSERT(selected.count() == 1);
    const int index = *selected.cbegin();
//  see Types.hpp for why 20
    Q_ASSERT(0 <= index && index < 20);

    QPoint &p = ui->autotileView->getMetatileAt(index);
    p.setX(ui->xSpinBox->value());
    p.setY(ui->ySpinBox->value());

    ui->autotileView->update();
}

void ImportAutoTileDialog::onMetatileChanged(const QPoint new_position)
{
    set_value(ui->xSpinBox, new_position.x());
    set_value(ui->ySpinBox, new_position.y());
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
