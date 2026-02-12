#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class BrushEditorDialog; }
QT_END_NAMESPACE

class BrushEditorDialog: public QDialog
{
    Q_OBJECT
public:
    explicit BrushEditorDialog(QImage pixels, QWidget *parent = nullptr);
    ~BrushEditorDialog();

    QImage getBrushPixels() const;

public slots:
    void setCursorPositionLabel(const QPoint position);

private:
    Ui::BrushEditorDialog *ui;
};
