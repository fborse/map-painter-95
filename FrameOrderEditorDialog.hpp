#pragma once

#include <QDialog>
#include <QUndoStack>

QT_BEGIN_NAMESPACE
namespace Ui { class FrameOrderEditorDialog; }
QT_END_NAMESPACE

#include "Types.hpp"

class FrameOrderTileSelectionWidget: public QWidget
{
    Q_OBJECT
public:
    explicit FrameOrderTileSelectionWidget(QWidget *parent = nullptr);

    void setTilesize(const int size) { tilesize = size; }

    void addSimpleTileCaption(const QString &id, const QImage &caption);
    void addAutoTileCaption(const QString &id, const QImage &caption);

    void resize();

public slots:
    void setNumberOfColumns(const int n) { n_columns = n; resize(); }

    void setSelectedTile(const TileReference ref) { selected_tile = ref; update(); }

signals:
    void tileClicked(const TileReference ref);

private:
    int tilesize;

    QVector<QPair<QString, QImage>> autotile_captions, simple_tile_captions;

    int n_columns;
    TileReference selected_tile;

    QPoint mouse_position;

    QSize getGridAspect() const;
    std::optional<TileReference> toRef(const QPoint &ij) const;

    void paintBackground(QPainter &painter);
    void paintCaptions(QPainter &painter);
    void paintGrid(QPainter &painter);
    void paintHoverCursor(QPainter &painter);
    void paintSelected(QPainter &painter);
    void paintEvent(QPaintEvent *) final override;

    void mouseMoveEvent(QMouseEvent *event) final override;
    void mousePressEvent(QMouseEvent *event) final override;
};

class FrameOrderEditorDialog: public QDialog
{
    Q_OBJECT
public:
    explicit FrameOrderEditorDialog(QWidget *parent = nullptr);
    ~FrameOrderEditorDialog();

    void setTilesize(const int size) { tilesize = size; }

    void setUndoStackPointer(QWeakPointer<QUndoStack> ptr) { undo_stack_ptr = ptr; }
    void setSimpleTilesOrderPointer(QWeakPointer<Names> ptr) { simple_tiles_order_ptr = ptr; }
    void setSimpleTilesPointer(QWeakPointer<SimpleTiles> ptr) { simple_tiles_ptr = ptr; }
    void setAutoTilesOrderPointer(QWeakPointer<Names> ptr) { autotiles_order_ptr = ptr; }
    void setAutoTilesPointer(QWeakPointer<AutoTiles> ptr) { autotiles_ptr = ptr; }

    void updateTileset();

public slots:
    void onAccept();

    void onSelectedTileChanged(const TileReference ref);
    void onSelectedFrameChanged(const int index);

    void onAddFrame();
    void onCloneFrame();
    void onRemoveFrame();
    void onMoveFrameUp();
    void onMoveFrameDown();

private:
    Ui::FrameOrderEditorDialog *ui;

    int tilesize;

    QWeakPointer<QUndoStack> undo_stack_ptr;
    QWeakPointer<Names> simple_tiles_order_ptr, autotiles_order_ptr;
    QWeakPointer<SimpleTiles> simple_tiles_ptr;
    QWeakPointer<AutoTiles> autotiles_ptr;

    TileReference selected_tile;

    int selected_frame;
};
