#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ImportTilesetDialog; }
QT_END_NAMESPACE

#include "Types.hpp"

class ImportTilesetDialog: public QDialog
{
    Q_OBJECT
public:
    explicit ImportTilesetDialog(QWidget *parent = nullptr);
    ~ImportTilesetDialog();

    void setTileSize(const int size) { tilesize = size; }

    AutoTiles getAddedAutoTiles() const;
    SimpleTiles getAddedSimpleTiles() const;

public slots:
    void onChangePath();
    void onAddAutoTiles();
    void onAddSimpleTiles();
    void onRemoveAutoTiles();
    void onRemoveSimpleTiles();

    void onAccept();

private:
    Ui::ImportTilesetDialog *ui;

    int tilesize;

    Names simple_tiles_order, autotiles_order;
    SimpleTiles simple_tiles;
    AutoTiles autotiles;

    QVector<QString> added_simple_tiles, added_autotiles;

    void updateAutoTilesSourceList();
    void updateSimpleTilesSourceList();
};
