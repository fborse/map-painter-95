#pragma once

#include "EditorWidget.hpp"

enum DrawTool
{
    PEN = 0,
    LINE = 1,
    BRUSH = 2,
    SHAPE = 3,
    FILL = 4,
    ERASER = 5,
    SHADER = 6,
    PIPETTE = 7,
    SELECTION = 8
};

enum SelectionShape
{
    RECTANGLE = 0,
    ELLIPSE = 1,
    MAGIC = 2
};

class MapPainterWidget final: public EditorWidget
{
    Q_OBJECT
public:
    explicit MapPainterWidget(QWidget *parent = nullptr);
    virtual ~MapPainterWidget() final override = default;

    QColor getDrawColor() const { return draw_color; }
    QImage getBrushPixels() const { return brush_pixels; }

    QImage getSelectionImage() const { return selection_image; }
    void setSelectionImage(const QImage &image) { selection_image = image; }
    void setSelectionRect(const QRect &rect) { selection_rect = rect; }

    void flipSelection(const bool horizontally, const bool vertically);
    void scaleSelection(const double fw, const double fh);
    void transposeSelection();
    void rotateSelection(const int angle);
    void cutSelection();
    void resetSelection() { selection_image = {}; selection_rect = {}; }

    void selectAll();

    void resize() final override;

public slots:
    void setZoom(const double z) override { EditorWidget::setZoom(z); redrawCursorImage(); }

    void setShowGrid(const bool yes) { show_grid = yes; update(); }
    void setDrawTool(const int index);
    void setRetroactive(const bool yes) { retroactive = yes; }

    void setDrawColor(const QColor color);

    void setPenSize(const int size) { pen_size = size; redrawCursorImage(); }
    void setAntiAliasing(const bool yes) { anti_aliasing = yes; }
    void setRoundPenCorners(const bool yes) { round_pen_corners = yes; redrawCursorImage(); }

    void setBrushPixels(const QImage pixels) { brush_pixels = pixels; redrawCursorImage(); }
//  this one comes from a combo box
    void setEllipseShape(const int yes) { ellipse_shape = (yes == 1); }
    void setFillShape(const bool yes) { fill_shape = yes; }
    void setRectRadius(const int radius) { rect_radius = radius; }

    void setFillTolerance(const double tolerance) { fill_tolerance = tolerance; }
    void setFillThisTileOnly(const bool yes) { fill_this_tile_only = yes; }

    void setDarken(const bool yes) { darken = yes; }
//  so does this one
    void setSelectionShape(const int index);
    void setSelectionColorKey(const bool yes);

signals:
    void colorChanged(const QColor color);
    void tilesAdded();

    void canCopy(bool);

private:
    bool show_grid;
    DrawTool draw_tool;
    bool retroactive;

    QColor draw_color;
    int pen_size;
    bool anti_aliasing;
    bool round_pen_corners;

    QImage brush_pixels;

    bool ellipse_shape;
    bool fill_shape;
    int rect_radius;

    double fill_tolerance;
    bool fill_this_tile_only;

    bool darken;

    SelectionShape selection_shape;
    bool selection_color_key;

    QPoint mouse_cursor;
//  right click is really just a desktop thing => names accordingly
    std::optional<QPoint> click_origin;
    bool right_click;
    bool shift_key;

    QVector<QPoint> drag_points;

    std::optional<QRect> selection_rect, original_rect;
    QImage selection_image, original_selection_image;
    std::optional<QPoint> move_offset;
//  TODO: QPainterPath may be better suited
    QVector<QPoint> magic_points;

    QImage cursor_image;

    QColor getEffectiveDrawColor() const;
//  the first can be useful for paintCursor (or just could ?)
    QPen getPen() const;
    void setPen(QPainter &painter) const;

    void drawPen(QPainter &painter) const;
    void drawLine(QPainter &painter) const;
    void drawBrush(QPainter &painter) const;
    void drawShape(QPainter &painter) const;
    void drawFill(QImage &original) const;
    void drawEraser(QPainter &painter) const;
    void drawShader(QPainter &painter) const;

    void drawSelectionPixels(QPainter &painter) const;
    void drawSelectionOutline(QPainter &painter) const;

    void redrawDisplayedSelectionImage();
    void redrawCursorImage();
    void paintCursor(QPainter &painter) const;

    QImage getDrawnLayer() const;
    void paintEvent(QPaintEvent *) override;

    QColor getColorAt(const QPoint &p) const;

//  TODO: find a more palatable structure than this ugly composite type
//  requirement is to aggregate the data as efficiently, though
    void handleRetroactiveDrawing(const QHash<QPoint, QHash<QPoint, QColor>> &changed_pixels);
    void handleNonRetroactiveDrawing(const QHash<QPoint, QHash<QPoint, QColor>> &changed_pixels);

    void blitSelection();
    void handleSelectionMade();
    void handleDrawChanges();

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
};
