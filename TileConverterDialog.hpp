#pragma once

#include <QDialog>
#include <QUndoStack>

QT_BEGIN_NAMESPACE
namespace Ui { class TileConverterDialog; }
QT_END_NAMESPACE

#include "Types.hpp"

class SelectionGridWidget: public QWidget
{
    Q_OBJECT
public:
    SelectionGridWidget(QWidget *parent = nullptr);

    void setTilesize(const int size) { tilesize = size; }
    virtual void resize() { setFixedSize(grid_aspect * tilesize); update(); }

signals:
    void tileSelected(const QString id);

protected:
    QSize grid_aspect;
    int tilesize;

    QPoint mouse_position;

    void setGridAspect(const QSize aspect) { grid_aspect = aspect; }
    void setGridAspect(const int m, const int n) { setGridAspect({m, n}); }

    QPoint toIJ(const int index) const;
    int toIndex(const QPoint &grid_coordinates) const;

    void paintBackground(QPainter &painter);
    void paintGrid(QPainter &painter);
    void paintSelectionRect(QPainter &painter, const QPoint &grid_coordinates);
    void paintHoverCursor(QPainter &painter, const bool valid_coordinates);

    virtual void mouseMoveEvent(QMouseEvent *event) override;
};

class TileSelectionWidget final: public SelectionGridWidget
{
    Q_OBJECT
public:
    explicit TileSelectionWidget(QWidget *parent = nullptr);

    void addCaption(const QString &id, const QImage &caption);
    void resize() final override;

public slots:
    void setNumberOfColumns(const int n) { n_columns = n; resize(); }

private:
    QVector<QPair<QString, QImage>> captions;

    int n_columns;
    int selected;

    void paintCaptions(QPainter &painter);
    void paintEvent(QPaintEvent *) final override;

    void mousePressEvent(QMouseEvent *event) final override;
};

class SimpleTileViewWidget final: public QWidget
{
    Q_OBJECT
public:
    explicit SimpleTileViewWidget(QWidget *parent = nullptr);

    void setMetaTileSize(const int size) { metatilesize = size; }
    void setTileCaption(const QImage &simple_tile);

    QRect getMetaTileRect() const;
    QImage getMetatile() const;

private:
    int metatilesize;
    QImage tile;

    QPoint metatile_coordinates;

    QPoint mouse_position;
    std::optional<QPoint> move_offset;

    void paintBackground(QPainter &painter);
    void paintMetatileCursor(QPainter &painter);
    void paintEvent(QPaintEvent *) final override;

    void mouseMoveEvent(QMouseEvent *event) final override;
    void mousePressEvent(QMouseEvent *event) final override;
    void mouseReleaseEvent(QMouseEvent *event) final override;
};

//  this widget also only stores captions as to
//  be usable for both simple and auto tiles
class GeneratedTilesWidget final: public SelectionGridWidget
{
    Q_OBJECT
public:
    explicit GeneratedTilesWidget(QWidget *parent = nullptr);

    void addCaption(const QString &id, const QImage &caption);
    void setNewTileCaption(const QImage &caption) { new_caption = caption; update(); }

    void removeCaption(const QString &id);

signals:
    void addTileClicked();
    void removeTileClicked(const QString id);

private:
    QVector<QPair<QString, QImage>> captions;
    QImage new_caption;

    int selected;

    void paintCaptions(QPainter &painter);
    void paintNewTileCaption(QPainter &painter);
    void paintAddSymbol(QPainter &painter);
    void paintEvent(QPaintEvent *) final override;

    void mousePressEvent(QMouseEvent *event) final override;
};

class MetatilesViewWidget final: public SelectionGridWidget
{
    Q_OBJECT
public:
    explicit MetatilesViewWidget(QWidget *parent = nullptr);

    void setMetatileOrigin(const int index, const QString &id, const QRect &rect);
    void setMetatileCaption(const int index, const QImage &pixels);

    QPair<QString, QRect> getMetatileOrigin(const int index) const { return origins[index]; }

signals:
    void metatileClicked(const int index);

private:
    QVector<QPair<QString, QRect>> origins;
    QVector<QImage> metatiles;

    void paintMetatiles(QPainter &painter);
    void paintEvent(QPaintEvent *) final override;

    void mousePressEvent(QMouseEvent *event) final override;
};

class MetatileSelectionWidget final: public SelectionGridWidget
{
    Q_OBJECT
public:
    explicit MetatileSelectionWidget(QWidget *parent = nullptr);

    void setMetatile(const int index, const QImage &metatile);

    Orientation getOrientation() const;
    QImage assembleCaption() const;

    int getTopLeft() const { return metatile_coordinates[top_left]; }
    int getTopRight() const { return metatile_coordinates[top_right]; }
    int getBottomLeft() const { return metatile_coordinates[bottom_left]; }
    int getBottomRight() const { return metatile_coordinates[bottom_right]; }

signals:
    void selectedMoved();

private:
    QVector<QImage> metatiles;

    QPoint top_left, top_right, bottom_left, bottom_right;

    enum Selected { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT};
    std::optional<Selected> dragged;

    static const QHash<QPoint, int> metatile_coordinates;

    void paintMetatiles(QPainter &painter);
    void paintDividers(QPainter &painter);
    void paintSelectionCursors(QPainter &painter);
    void paintHoverCursor(QPainter &painter);
    void paintEvent(QPaintEvent *) final override;

    void mouseMoveEvent(QMouseEvent *event) final override;
    void mousePressEvent(QMouseEvent *event) final override;
    void mouseReleaseEvent(QMouseEvent *event) final override;
};

class TileConverterDialog: public QDialog
{
    Q_OBJECT
public:
    explicit TileConverterDialog(QWidget *parent = nullptr);
    ~TileConverterDialog();

    void setTilesize(const int size) { tilesize = size; }

    void setUndoStackPointer(QWeakPointer<QUndoStack> ptr) { undo_stack_ptr = ptr; }
    void setSimpleTilesOrderPointer(QWeakPointer<Names> ptr) { simple_tiles_order_ptr = ptr; }
    void setSimpleTilesPointer(QWeakPointer<SimpleTiles> ptr) { simple_tiles_ptr = ptr; }
    void setAutoTilesOrderPointer(QWeakPointer<Names> ptr) { autotiles_order_ptr = ptr; }
    void setAutoTilesPointer(QWeakPointer<AutoTiles> ptr) { autotiles_ptr = ptr; }

    void updateSelectionWidgets();

public slots:
    void onSelectedSimpleTileChanged(const QString id);
    void onMetaTileClicked(const int index);
    void onAddAutoTile();
    void onRemoveAutoTile(const QString id);

    void onSelectedAutoTileChanged(const QString id);
    void onAddSimpleTile();
    void onRemoveSimpleTile(const QString id);
    void onUpdateGeneratedSimpleTile();

private:
    Ui::TileConverterDialog *ui;

    int tilesize;

    QWeakPointer<QUndoStack> undo_stack_ptr;
    QWeakPointer<Names> simple_tiles_order_ptr, autotiles_order_ptr;
    QWeakPointer<SimpleTiles> simple_tiles_ptr;
    QWeakPointer<AutoTiles> autotiles_ptr;

    QString selected_simple_tile, selected_autotile;
    QVector<QPair<QString, SimpleTile>> added_simple_tiles;
    QVector<QPair<QString, AutoTile>> added_autotiles;
    QHash<QString, QVector<QPair<QString, QRect>>> added_metatile_origins;

    void updateMetatileSelectionWidget();

    void updateSimpleTileSelectionWidget();
    void updateAutoTileSelectionWidget();
};
