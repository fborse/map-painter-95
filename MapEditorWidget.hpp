#pragma once

#include "EditorWidget.hpp"

class MapEditorWidget final: public EditorWidget
{
    Q_OBJECT
public:
    explicit MapEditorWidget(QWidget *parent = nullptr);
    virtual ~MapEditorWidget() final override = default;

    void resize() final override;
    void resizeMap(const QSize &size);

    void insertLayer(const int index);
    void removeLayer(const int index);

public slots:
    void setShowAboveLayers(const bool yes) { show_above_layers = yes; update(); }
    void setReorientAutoTiles(const bool yes) { reorient_autotiles = yes; }

signals:
//  only purposes is to tell MainWindow to refresh the relevant editor widgets
    void tileSelected();
    void tilesSet();
    void mapResized();

private:
    bool show_above_layers;
    bool reorient_autotiles;

    QPoint mouse_cursor;
//  right click is really just a desktop thing => names accordingly
    std::optional<QPoint> click_origin, right_click_origin;

    void paintSimpleTile(QPainter &painter, const TileReference &id, const QPoint &p);
    void paintAutoTile(QPainter &painter, const TileReference &ref, const QPoint &p);
    void paintTileRects(QPainter &painter);
    void paintRectOutlines(QPainter &painter);

    void paintEvent(QPaintEvent *) override;

//  autotiles can involve changes outside of the drawing rect
    QHash<QPoint, TileReference> getDrawnTiles() const;

    void handleTileSetting();
    void handleTileSelection();

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};
