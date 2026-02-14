#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class AddTileDialog; }
QT_END_NAMESPACE

class AddTileDialog: public QDialog
{
    Q_OBJECT
public:
    explicit AddTileDialog(const int tilesize, QWidget *parent = nullptr);
    ~AddTileDialog();

    QColor getColor() const { return current; }

public slots:
    void onSelectColor();
    void onColorSelected(const QColor color);

signals:
    void colorSelected(const QColor color);

private:
    Ui::AddTileDialog *ui;

    int tilesize;
    QColor current;
};
