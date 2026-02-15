#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ResizeMapDialog; }
QT_END_NAMESPACE

class ResizeMapDialog: public QDialog
{
    Q_OBJECT
public:
    explicit ResizeMapDialog(QWidget *parent = nullptr);
    ~ResizeMapDialog();

    void setSize(const QSize &size);
    QSize getSize() const;

private:
    Ui::ResizeMapDialog *ui;
};
