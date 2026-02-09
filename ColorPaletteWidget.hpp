#pragma once

#include <QWidget>

class ColorPaletteWidget: public QWidget
{
    Q_OBJECT
public:
    explicit ColorPaletteWidget(QWidget *parent = nullptr);

public slots:
    void addColor(const QColor color);
    void selectColor(const QColor color);

signals:
    void colorSelected(const QColor color);

private:
    static const int RECT_SIZE = 16;

    QVector<QColor> colors;
    int selected;

    std::optional<int> click_origin;

    int getNumberOfColumns() const { return width() / RECT_SIZE; }
    std::optional<int> getIndexOf(const QPoint &p) const;

    void paintEvent(QPaintEvent *) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};
