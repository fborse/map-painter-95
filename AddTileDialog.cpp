#include "AddTileDialog.hpp"
#include "ui_AddTileDialog.h"

#include <QColorDialog>
#include <QPainter>

AddTileDialog::AddTileDialog(const int tilesize, QWidget *parent):
    QDialog(parent), ui(new Ui::AddTileDialog),
    tilesize{tilesize}, current{}
{
    ui->setupUi(this);

    onColorSelected(Qt::transparent);
}

AddTileDialog::~AddTileDialog()
{
    delete ui;
}

void AddTileDialog::onSelectColor()
{
    const QColor color = QColorDialog::getColor(current, this);
    if (color.isValid() && color != current)
        emit colorSelected(color);
}

static inline void draw_background(QPainter &painter, const int tilesize)
{
    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};

    painter.fillRect(0, 0, tilesize/2, tilesize/2, dark);
    painter.fillRect(tilesize/2, 0, qCeil(tilesize/2.0), tilesize/2, light);
    painter.fillRect(0, tilesize/2, tilesize/2, qCeil(tilesize/2.0), light);
    painter.fillRect(tilesize/2, tilesize/2, qCeil(tilesize/2.0), qCeil(tilesize/2.0), dark);
}

void AddTileDialog::onColorSelected(const QColor color)
{
    current = color;

    ui->colorSelectionWidget->setColor(current);

    QImage image(tilesize, tilesize, QImage::Format_ARGB32_Premultiplied);
    {
        QPainter painter(&image);
        draw_background(painter, tilesize);
        painter.fillRect(0, 0, tilesize, tilesize, current);
    }
    ui->tileViewLabel->setPixmap(QPixmap::fromImage(image));
}
