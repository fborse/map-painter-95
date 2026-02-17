#include "ScaleSelectionDialog.hpp"
#include "ui_ScaleSelectionDialog.h"

enum ScaleMode { FACTOR = 0, SIZE = 1 };

static inline void set_value(QSpinBox *box, const int value)
{
    box->blockSignals(true);
    box->setValue(value);
    box->blockSignals(false);
}

ScaleSelectionDialog::ScaleSelectionDialog(const QSize &original_size, QWidget *parent):
    QDialog(parent), ui(new Ui::ScaleSelectionDialog),
    original_size{original_size}, preserve_ratio{true}, ratio{}
{
    ui->setupUi(this);

    ui->widthSpinBox->setValue(original_size.width());
    ui->heightSpinBox->setValue(original_size.height());
    ratio = getRatio();

    connect(ui->widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [&] {
        if (preserve_ratio)
        {
            const double w = ui->widthSpinBox->value();
            set_value(ui->heightSpinBox, w * ratio);
        }
    });

    connect(ui->heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [&] {
        if (preserve_ratio)
        {
            const double h = ui->heightSpinBox->value();
            set_value(ui->widthSpinBox, h / ratio);
        }
    });
}

ScaleSelectionDialog::~ScaleSelectionDialog()
{
    delete ui;
}

double ScaleSelectionDialog::getRatio() const
{
    const double w = ui->widthSpinBox->value();
    const double h = ui->heightSpinBox->value();

    return h / w;
}

bool ScaleSelectionDialog::isInFactorMode() const
{
    return (ScaleMode(ui->tabWidget->currentIndex()) == FACTOR);
}

double ScaleSelectionDialog::getHorizontalFactor() const
{
    const double h = ui->horizontallyDoubleSpinBox->value();
    const double w = ui->widthSpinBox->value();
    const double w0 = original_size.width();

    if (isInFactorMode())
        return h;
    else
        return w / w0;
}

double ScaleSelectionDialog::getVerticalFactor() const
{
    const double v = ui->verticallyDoubleSpinBox->value();
    const double h = ui->heightSpinBox->value();
    const double h0 = original_size.height();

    if (isInFactorMode())
        return v;
    else
        return h / h0;
}
