#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ScaleSelectionDialog; }
QT_END_NAMESPACE

class ScaleSelectionDialog: public QDialog
{
    Q_OBJECT
public:
    explicit ScaleSelectionDialog(const QSize &original_size, QWidget *parent = nullptr);
    ~ScaleSelectionDialog();

    bool isInFactorMode() const;

    double getHorizontalFactor() const;
    double getVerticalFactor() const;

public slots:
    void setPreserveRatio(const bool yes) { preserve_ratio = yes; ratio = getRatio(); }

private:
    Ui::ScaleSelectionDialog *ui;

    QSize original_size;
    bool preserve_ratio;
    double ratio;

    double getRatio() const;
};
