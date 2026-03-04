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

struct TileReference
{
    QString name;
    bool autotile = false;

    bool isEmpty() const { return name.isEmpty(); }
    operator bool() const { return isEmpty(); }

    bool operator==(const TileReference &o) const { return name == o.name && autotile == o.autotile; }
    bool operator!=(const TileReference &o) const { return !(*this == o); }
};

inline uint qHash(const TileReference &ref, uint seed = 0)
{
    return seed ^ (qHash(ref.name, seed) * 31 + qHash(ref.autotile, seed) * 37);
}

using SelectedTiles = QVector<QVector<TileReference>>;

using MapLayer = QVector<QVector<TileReference>>;
using MapLayers = QVector<MapLayer>;
