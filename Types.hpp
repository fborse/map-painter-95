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

struct Orientation
{
//  see below for why this makes an isolated autotile
    int top_left = 0, top_right = 3;
    int bottom_left = 12, bottom_right = 15;

    bool operator==(const Orientation &other) const
    {
        return (top_left == other.top_left)
            && (top_right == other.top_right)
            && (bottom_left == other.bottom_left)
            && (bottom_right == other.bottom_right);
    }
};

inline uint qHash(const Orientation &orientation, const uint seed = 0)
{
    return seed ^ (
        qHash(orientation.top_left, seed) * 31
      + qHash(orientation.top_right, seed) * 37
      + qHash(orientation.bottom_left, seed) * 41
      + qHash(orientation.bottom_right, seed) * 43
    );
}

struct AutoTile
{
    struct Frame
    {
    //  RPG Maker scheme but joints are at the bottom in a row
    //  and displayed tile in the editor is made of corners
    //  top-left        top2            top1                top-right
    //  left2           middle4         middle3             right2
    //  left1           middle2         middle1             right1
    //  bottom-left     bottom2         bottom1             bottom-right
    //  top-left-joint  top-right-joint bottom-left-joint   bottom-right-joint
        QVector<QImage> metatiles;

        QImage genTile(const Orientation &orientation) const;
    };

    QVector<Frame> frames;
};

using AutoTiles = QHash<QString, AutoTile>;

struct TileReference
{
    QString name;
    bool autotile = false;
    Orientation orientation;

    bool isEmpty() const { return name.isEmpty(); }
    operator bool() const { return !isEmpty(); }

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
