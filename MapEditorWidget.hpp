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

public slots:
    void setShowAboveLayers(const bool yes) { show_above_layers = yes; update(); }

signals:
//  only purposes is to tell MainWindow to refresh the relevant editor widgets
    void tileSelected();
    void tilesSet();
    void mapResized();

private:
    bool show_above_layers;

    QPoint mouse_cursor;
//  right click is really just a desktop thing => names accordingly
    std::optional<QPoint> click_origin, right_click_origin;

    void paintTileRects(QPainter &painter);
    void paintRectOutlines(QPainter &painter);

    void paintEvent(QPaintEvent *) override;

    void handleTileSetting();
    void handleTileSelection();

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};
