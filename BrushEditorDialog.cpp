#include "BrushEditorDialog.hpp"
#include "ui_BrushEditorDialog.h"

BrushEditorDialog::BrushEditorDialog(QImage pixels, QWidget *parent):
    QDialog(parent), ui(new Ui::BrushEditorDialog)
{
    ui->setupUi(this);

    Q_ASSERT(!pixels.isNull());
    ui->brushEditorWidget->setBrushPixels(pixels);
    Q_ASSERT(pixels.width() > 0);
    ui->widthSpinBox->setValue(pixels.width());
    Q_ASSERT(pixels.height() > 0);
    ui->heightSpinBox->setValue(pixels.height());
}

BrushEditorDialog::~BrushEditorDialog()
{
    delete ui;
}

QImage BrushEditorDialog::getBrushPixels() const
{
    QImage image = ui->brushEditorWidget->getBrushPixels();
    Q_ASSERT(!image.isNull());

    return image;
}

void BrushEditorDialog::setCursorPositionLabel(const QPoint position)
{
    const auto &[x, y] = position;
    const int zoom = ui->zoomDoubleSpinBox->value();

    ui->cursorPositionLabel->setText(QString("(%1, %2)").arg(x / zoom).arg(y / zoom));
}
