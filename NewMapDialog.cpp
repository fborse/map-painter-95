#include "NewMapDialog.hpp"
#include "ui_NewMapDialog.h"

#include <QMessageBox>
#include <QFileDialog>

NewMapDialog::NewMapDialog(QWidget *parent):
    QDialog(parent), ui(new Ui::NewMapDialog)
{
    ui->setupUi(this);

    connect(ui->pathPushButton, &QPushButton::clicked, [&] {
        const char *title = "Choose a Map Painter 95 tileset to import !";
        const char *ext = "Tilesets (*.ts95)";

        const QString path = QFileDialog::getOpenFileName(this, tr(title), QString(), tr(ext));
        if (!path.isEmpty())
            ui->pathLineEdit->setText(path);
    });

    enableImportTileset(false);
}

NewMapDialog::~NewMapDialog()
{
    delete ui;
}

void NewMapDialog::enableFromScratch(const bool enabled)
{
    ui->tilesizeLabel->setEnabled(enabled);
    ui->tilesizeSpinBox->setEnabled(enabled);
    ui->addDefaultTilesCheckBox->setEnabled(enabled);
}

void NewMapDialog::enableImportTileset(const bool enabled)
{
    ui->pathLabel->setEnabled(enabled);
    ui->pathLineEdit->setEnabled(enabled);
    ui->pathPushButton->setEnabled(enabled);
}

void NewMapDialog::onAccept()
{
    const char *title = "Cannot create the map yet !";
    const char *msg = "Cannot import a tileset without giving a path to a tileset.";

    if (shouldImportTileset() && getImportPath().isEmpty())
        QMessageBox::information(this, tr(title), tr(msg));
    else
        accept();
}

bool NewMapDialog::shouldImportTileset() const
{
    return ui->importRadioButton->isChecked();
}

int NewMapDialog::getTilesize() const
{
    return ui->tilesizeSpinBox->value();
}

bool NewMapDialog::shouldCreateDefaultTiles() const
{
    return ui->addDefaultTilesCheckBox->isChecked();
}

QString NewMapDialog::getImportPath() const
{
    return ui->pathLineEdit->text();
}

QSize NewMapDialog::getMapSize() const
{
    const int w = ui->widthSpinBox->value();
    const int h = ui->heightSpinBox->value();

    return {w, h};
}
