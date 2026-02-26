#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class AddRectDialog; }
QT_END_NAMESPACE

class AddRectDialog: public QDialog
{
    Q_OBJECT
public:
    explicit AddRectDialog(const int tilesize, QWidget *parent = nullptr);
    ~AddRectDialog();

    QColor getColor() const { return current; }

public slots:
    void onSelectColor();
    void onColorSelected(const QColor color);

signals:
    void colorSelected(const QColor color);

private:
    Ui::AddRectDialog *ui;

    int tilesize;
    QColor current;
};
