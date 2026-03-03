#pragma once

#include <QVector>
#include <QHash>
#include <QImage>

using Names = QVector<QString>;
using SimpleTile = QVector<QImage>;
using Tileset = QHash<QString, SimpleTile>;
using SelectedTiles = QVector<QVector<QString>>;

using TileReference = QString;
using MapLayer = QVector<QVector<TileReference>>;
using MapLayers = QVector<MapLayer>;
