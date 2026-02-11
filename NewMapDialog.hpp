#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class NewMapDialog; }
QT_END_NAMESPACE

class NewMapDialog: public QDialog
{
    Q_OBJECT
public:
    explicit NewMapDialog(QWidget *parent = nullptr);
    ~NewMapDialog();

    bool shouldImportTileset() const;

    int getTilesize() const;
    bool shouldCreateDefaultTiles() const;

    QString getImportPath() const;

    QSize getMapSize() const;

public slots:
    void enableFromScratch(const bool enabled);
    void enableImportTileset(const bool enabled);

    void onAccept();

private:
    Ui::NewMapDialog *ui;
};
