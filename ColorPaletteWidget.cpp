#include "ColorPaletteWidget.hpp"

#include <QPainter>
#include <QMouseEvent>

ColorPaletteWidget::ColorPaletteWidget(QWidget *parent):
    QWidget(parent),
    colors{}, selected{-1},
    click_origin{}
{
    addColor(QColor(Qt::black).toHsv());
    addColor(QColor(Qt::white).toHsv());
}

void ColorPaletteWidget::resetPalette()
{
    colors.clear();

    addColor(QColor(Qt::black).toHsv());
    addColor(QColor(Qt::white).toHsv());

    update();
}

std::optional<QColor> ColorPaletteWidget::getSelectedColor() const
{
    if (0 <= selected && selected < colors.length())
        return colors[selected];
    else
        return {};
}

void ColorPaletteWidget::addColor(const QColor color)
{
    if (!colors.contains(color))
        colors.push_back(color);

    update();
}

void ColorPaletteWidget::removeSelectedColor()
{
    Q_ASSERT(0 <= selected && selected < colors.length());
    colors.remove(selected);

    if (selected >= colors.length())
    {
        selected = -1;
        emit canRemoveSelected(false);
    }
}

void ColorPaletteWidget::selectColor(const QColor color)
{
    selected = colors.indexOf(color);
    emit canRemoveSelected(selected >= 0);
    update();
}

void ColorPaletteWidget::drawBackground(QPainter &painter)
{
    const QColor dark = {64, 64, 64};
    const QColor light = {128, 128, 128};

    const int ncol = getNumberOfColumns();
    const int s = RECT_SIZE;

    for (int i = 0; i < colors.length(); ++i)
    {
        const int x = (i % ncol) * s;
        const int y = (i / ncol) * s;

        painter.fillRect(x, y, s/2, s/2, dark);
        painter.fillRect(x + s/2, y, s/2, s/2, light);
        painter.fillRect(x, y + s/2, s/2, s/2, light);
        painter.fillRect(x + s/2, y + s/2, s/2, s/2, dark);
    }
}

void ColorPaletteWidget::drawColors(QPainter &painter)
{
    const int ncol = getNumberOfColumns();
    const QSize s = {RECT_SIZE, RECT_SIZE};

    for (int i = 0; i < colors.length(); ++i)
    {
        const QPoint p = QPoint(i % ncol, i / ncol) * RECT_SIZE;
        painter.fillRect(QRect(p, s), colors[i]);
    }
}

void ColorPaletteWidget::drawOutlines(QPainter &painter)
{
    const int ncol = getNumberOfColumns();
    const QSize s = {RECT_SIZE, RECT_SIZE};

//  inner and outer rect have slight differences
    const QPoint dp = {1, 1};
    const QSize ds = {1, 1};

    for (int i = 0; i < colors.length(); ++i)
    {
        const QPoint p = QPoint(i % ncol, i / ncol) * RECT_SIZE;

        painter.setPen((i == selected)? Qt::white : Qt::black);
        painter.drawRect(QRect(p, s - ds));

        painter.setPen(Qt::black);
        painter.drawRect(QRect(p + dp, s - 3*ds));
    }
}

void ColorPaletteWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    drawBackground(painter);
    drawColors(painter);
    drawOutlines(painter);
}

std::optional<int> ColorPaletteWidget::getIndexOf(const QPoint &p) const
{
    const int n_columns = getNumberOfColumns();
    const int i = p.x() / RECT_SIZE;
    const int j = p.y() / RECT_SIZE;

    const int idx = i + j * n_columns;
    if (0 <= idx && idx < colors.length())
        return idx;
    else
        return {};
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
    {
        if (const auto idx = getIndexOf(event->pos()))
        {
            colors.remove(*idx);
            emit canRemoveSelected(0 <= selected && selected < colors.length());
        }
    }

    update();
}
