#pragma once

#include "EditorWidget.hpp"

class MapPainterWidget final: public EditorWidget
{
    Q_OBJECT
public:
    explicit MapPainterWidget(QWidget *parent = nullptr);
    virtual ~MapPainterWidget() final override = default;

private:
    QPoint mouse_cursor;
    std::optional<QPoint> click_origin;

    void paintEvent(QPaintEvent *) override;
};
