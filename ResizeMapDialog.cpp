#include "ResizeMapDialog.hpp"
#include "ui_ResizeMapDialog.h"

ResizeMapDialog::ResizeMapDialog(QWidget *parent):
    QDialog(parent), ui(new Ui::ResizeMapDialog)
{
    ui->setupUi(this);
}

ResizeMapDialog::~ResizeMapDialog()
{
    delete ui;
}

void ResizeMapDialog::setSize(const QSize &size)
{
    ui->widthSpinBox->setValue(size.width());
    ui->heightSpinBox->setValue(size.height());
}

QSize ResizeMapDialog::getSize() const
{
    const int w = ui->widthSpinBox->value();
    const int h = ui->heightSpinBox->value();

    return {w, h};
}
