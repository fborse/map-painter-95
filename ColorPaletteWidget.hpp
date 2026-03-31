#pragma once

#include <QWidget>

class ColorPaletteWidget: public QWidget
{
    Q_OBJECT
public:
    explicit ColorPaletteWidget(QWidget *parent = nullptr);

    void resetPalette();

    std::optional<QColor> getSelectedColor() const;

    QVector<QColor> getColors() const { return colors; }
    void setColors(const QVector<QColor> palette) { colors = palette; selected = -1; update(); }

public slots:
//  remember: used also by the constructor and resetPalette()
    void addColor(const QColor color);
    void removeSelectedColor();
    void selectColor(const QColor color);

signals:
    void colorSelected(const QColor color);
    void canRemoveSelected(const bool yes);

private:
    static const int RECT_SIZE = 16;

    QVector<QColor> colors;
    int selected;

    std::optional<int> click_origin;

    int getNumberOfColumns() const { return width() / RECT_SIZE; }
    std::optional<int> getIndexOf(const QPoint &p) const;

    void drawBackground(QPainter &painter);
    void drawColors(QPainter &painter);
    void drawOutlines(QPainter &painter);
    void paintEvent(QPaintEvent *) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};
