#include "ColorPaletteWidget.hpp"

#include <QPainter>
#include <QMouseEvent>

ColorPaletteWidget::ColorPaletteWidget(QWidget *parent):
    QWidget(parent),
    colors{}, selected{-1},
    click_origin{}
{
    addColor(Qt::black);
    addColor(Qt::white);
}

void ColorPaletteWidget::addColor(const QColor color)
{
    if (!colors.contains(color))
        colors.push_back(color);

    update();
}

void ColorPaletteWidget::selectColor(const QColor color)
{
    selected = colors.indexOf(color);
    update();
}

void ColorPaletteWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    const int n_columns = getNumberOfColumns();

    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};

    for (int i = 0; i < colors.length(); ++i)
    {
        const int x = (i % n_columns) * RECT_SIZE;
        const int y = (i / n_columns) * RECT_SIZE;

        const int s = RECT_SIZE;

    //  checkered background
        painter.fillRect(x, y, s/2, s/2, dark);
        painter.fillRect(x + s/2, y, s/2, s/2, light);
        painter.fillRect(x, y + s/2, s/2, s/2, light);
        painter.fillRect(x + s/2, y + s/2, s/2, s/2, dark);

    //  actual colour
        painter.fillRect(x, y, s, s, colors[i]);

    //  surrounding rects
        painter.setPen((i == selected)? Qt::white : Qt::black);
        painter.drawRect(x, y, s-1, s-1);
        painter.setPen(Qt::black);
        painter.drawRect(x+1, y+1, s-3, s-3);
    }
}

std::optional<int> ColorPaletteWidget::getIndexOf(const QPoint &p) const
{
    const int n_columns = getNumberOfColumns();
    const int i = p.x() / RECT_SIZE;
    const int j = p.y() / RECT_SIZE;

    const int idx = i + j * n_columns;
    if (idx < 0 || idx >= colors.length())
        return {};
    else
        return idx;
}

void ColorPaletteWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        click_origin = getIndexOf(event->pos());
}

void ColorPaletteWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (const auto idx = getIndexOf(event->pos()))
            if (*idx != selected)
                emit colorSelected(colors[selected = *idx]);

        click_origin = {};
    }

//  TODO: find cheat for non-desktop to access this ability
    if (event->button() == Qt::RightButton)
        if (const auto idx = getIndexOf(event->pos()))
            colors.remove(*idx);

    update();
}
