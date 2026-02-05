#pragma once

#include "EditorWidget.hpp"

class MapPainterWidget final: public EditorWidget
{
    Q_OBJECT
public:
    explicit MapPainterWidget(QWidget *parent = nullptr);
    virtual ~MapPainterWidget() final override = default;

private:
    void paintEvent(QPaintEvent *) override;
};
