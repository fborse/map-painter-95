#pragma once

#include "EditorWidget.hpp"

class MapEditorWidget final: public EditorWidget
{
    Q_OBJECT
public:
    explicit MapEditorWidget(QWidget *parent = nullptr);
    virtual ~MapEditorWidget() final override = default;

private:
    QPoint mouse_cursor;
    std::optional<QPoint> left_click, right_click;

    void paintLayers(QPainter &painter);
    void paintSelectionRect(QPainter &painter);

    void paintEvent(QPaintEvent *) override;

    void handleTileSelection();

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};
