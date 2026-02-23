#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ExportAsTexturesDialog; }
QT_END_NAMESPACE

using Tileset = QHash<QString, QImage>;

using TileReference = QString;
using MapLayer = QVector<QVector<TileReference>>;

class ExportAsTexturesDialog: public QDialog
{
    Q_OBJECT
public:
    explicit ExportAsTexturesDialog(const int tilesize, QWidget *parent = nullptr);
    ~ExportAsTexturesDialog();

    void setTilesetPointer(QWeakPointer<Tileset> ptr) { tileset = ptr; }
    void setMapLayersPointer(QWeakPointer<MapLayer> ptr) { map_layers = ptr; redrawLayers(); }

    QString getDirectory() const;
    QString getPattern() const;

//  TODO: Implement in a distant future
    int getNumberOfLayers() const;
    int getNumberOfFrames() const;

public slots:
    void onAccept();

private:
    Ui::ExportAsTexturesDialog *ui;

    int tilesize;
    QImage drawn_layers;

    QWeakPointer<Tileset> tileset;
    QWeakPointer<MapLayer> map_layers;

    void redrawLayers();
};
