#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ImportAutoTileDialog; }
QT_END_NAMESPACE

#include "Types.hpp"

class AutoTileViewWidget final: public QWidget
{
    Q_OBJECT
public:
    explicit AutoTileViewWidget(QWidget *parent = nullptr);

    void setTilesize(const int size) { tilesize = size; resize(); }
    void setMetatileImage(const int index, const QImage &image);

    QSet<int> getSelectedMetatiles() const;

public slots:
    void setMetatileState(const int index, const bool state);
    void clearSelections();

signals:
    void metatileClicked(const int index);
    void selectionChanged();

private:
    int tilesize;
    QVector<QImage> metatiles;
    QVector<bool> selections;

    void resize() { setFixedSize(4 * tilesize/2, 6 * tilesize/2); }

    void drawBackground(QPainter &painter);
    void drawMetatiles(QPainter &painter);
    void drawGrid(QPainter &painter);
    void drawSelectionRects(QPainter &painter);
    void paintEvent(QPaintEvent *) final override;

    void mousePressEvent(QMouseEvent *event) final override;
};

class ImportAutoTileWidget final: public QWidget
{
    Q_OBJECT
public:
    explicit ImportAutoTileWidget(QWidget *parent = nullptr);

    void setTexture(const QString &path);
    void setTilesize(const int size);
    QVector<QPoint> getMetatiles() const { return metatiles; }

    QPoint &getMetatileAt(const int index);
    const QImage &getDisplayedTexture() const { return displayed_texture; }

    AutoTile getAutoTile() const;

    void setSelectedMetatiles(const QSet<int> &selections) { selected = selections; update(); }

public slots:
    void setZoom(const double z) { zoom = z; updateDisplayedTexture(); }
    void setScaling(const double s) { scaling = s; updateDisplayedTexture(); }
    void setColorKey(const QColor color) { color_key = color; updateDisplayedTexture(); }
    void setMagnetic(const bool yes) { magnetic = yes; update(); }

    void addSelectedMetatile(const int index) { selected.insert(index); update(); }
    void clearSelectedMetatiles() { selected.clear(); update(); }

    void setMetatilePosition(const int index, const QPoint new_position);

signals:
    void metatileClicked(const int index);
    void clearSelection();

    void metatilePositionChanged(const int index, const QPoint new_position);
    void displayedTextureChanged();

private:
    int tilesize;
    QImage original_texture;
    QVector<QPoint> metatiles;

    double zoom, scaling;
    QColor color_key;
    bool magnetic;

    QSet<int> selected;
    QImage displayed_texture;

    QPoint mouse_cursor;
    std::optional<QPoint> rect_origin;
    QHash<int, QPoint> move_origins;

    void updateDisplayedTexture();

    void drawBackground(QPainter &painter);
    void drawMetatileRects(QPainter &painter);
    void drawGrid(QPainter &painter);
    void drawSelectionOutline(QPainter &painter);
    void drawSnapPoints(QPainter &painter, const int unit);
    void paintEvent(QPaintEvent *) final override;

    void mouseMoveEvent(QMouseEvent *event) final override;
    void mousePressEvent(QMouseEvent *event) final override;
    void mouseReleaseEvent(QMouseEvent *event) final override;
};

class ImportAutoTileDialog: public QDialog
{
    Q_OBJECT
public:
    explicit ImportAutoTileDialog(const int tilesize, QWidget *parent = nullptr);
    ~ImportAutoTileDialog();

    QString getTexturePath() const;

    AutoTile getAutoTile() const;

public slots:
    void onChangeTexturePath();
    void onChangeColorKey();

    void onAccept();

    void enableMetatilesWidgets(const bool enabled);
    void onSelectedMetatilesChanged();

    void enableMetatileWidgets(const bool enabled);

//  updating AutoTileViewWidget's metatile captions
    void onMetatilePositionChanged(const int index, const QPoint new_position);

    void onChangeMetatile();
    void onMetatileChanged(const QPoint new_position);

    void redrawMetatiles();

private:
    Ui::ImportAutoTileDialog *ui;

    int tilesize;
};
