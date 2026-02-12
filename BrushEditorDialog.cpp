#include "BrushEditorDialog.hpp"
#include "ui_BrushEditorDialog.h"

BrushEditorDialog::BrushEditorDialog(QImage pixels, QWidget *parent):
    QDialog(parent), ui(new Ui::BrushEditorDialog)
{
    ui->setupUi(this);

    ui->brushEditorWidget->setBrushPixels(pixels);
    ui->widthSpinBox->setValue(pixels.width());
    ui->heightSpinBox->setValue(pixels.height());
}

BrushEditorDialog::~BrushEditorDialog()
{
    delete ui;
}

QImage BrushEditorDialog::getBrushPixels() const
{
    return ui->brushEditorWidget->getBrushPixels();
}

void BrushEditorDialog::setCursorPositionLabel(const QPoint position)
{
    const auto &[x, y] = position;
    const int zoom = ui->zoomDoubleSpinBox->value();

    ui->cursorPositionLabel->setText(QString("(%1, %2)").arg(x / zoom).arg(y / zoom));
}
