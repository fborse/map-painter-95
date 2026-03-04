#pragma once

#include <QVector>
#include <QHash>
#include <QImage>

struct SimpleTile
{
    QVector<QImage> frames;
};

using Names = QVector<QString>;
using SimpleTiles = QHash<QString, SimpleTile>;

using SelectedTiles = QVector<QVector<QString>>;

using TileReference = QString;
using MapLayer = QVector<QVector<TileReference>>;
using MapLayers = QVector<MapLayer>;
