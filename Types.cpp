#include "Types.hpp"

#include <QPainter>

QImage AutoTile::Frame::genTile(const Orientation &orientation) const
{
    const auto &[tl, tr, bl, br] = orientation;
    Q_ASSERT(0 <= tl && tl < metatiles.length());
    Q_ASSERT(0 <= tr && tr < metatiles.length());
    Q_ASSERT(0 <= bl && bl < metatiles.length());
    Q_ASSERT(0 <= br && br < metatiles.length());

    const auto &[w, h] = metatiles[tl].size();
    QImage tile(2*w, 2*h, QImage::Format_ARGB32_Premultiplied);
    tile.fill(Qt::transparent);

    QPainter painter(&tile);
    painter.drawImage(0, 0, metatiles[tl]);
    painter.drawImage(w, 0, metatiles[tr]);
    painter.drawImage(0, h, metatiles[bl]);
    painter.drawImage(w, h, metatiles[br]);

    return tile;
}
