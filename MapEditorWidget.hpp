#pragma once

#include "EditorWidget.hpp"

class MapEditorWidget final: public EditorWidget
{
    Q_OBJECT
public:
    explicit MapEditorWidget(QWidget *parent = nullptr);
    virtual ~MapEditorWidget() final override = default;

private:
    void paintEvent(QPaintEvent *) override;
};
