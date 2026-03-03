#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ImportTilesInBulkDialog; }
QT_END_NAMESPACE

#include "Tileset.hpp"

struct ImportTilesArea { int x, y, w, h, dx, dy; };

class ImportTilesInBulkWidget final: public QWidget
{
    Q_OBJECT
public:
    ImportTilesInBulkWidget(QWidget *parent = nullptr);

    void setTexture(const QString &path);
    void setTilesize(const int size) { tilesize = size; update(); }
    QVector<ImportTilesArea> getAreas() const { return areas; }

    void addArea(const ImportTilesArea &area) { areas.push_back(area); update(); }
    void removeAreaAt(const int index);
    ImportTilesArea &getAreaAt(const int index);

    QVector<SimpleTile> getTiles() const;

public slots:
    void setZoom(const double z) { zoom = z; updateDisplayedTexture(); }
    void setScaling(const double s) { scaling = s; updateDisplayedTexture(); }
    void setColorKey(const QColor color) { color_key = color; updateDisplayedTexture(); }
    void setMagnetic(const bool yes) { magnetic = yes; update(); }

    void setSelectedArea(const int index) { selected = index; update(); }

signals:
    void areaSelected(const int index);
    void areaAdded(const ImportTilesArea area);
    void areaRemoved(const int index);
    void areaChanged(const QPoint new_position);

private:
    int tilesize;
    QImage original_texture;
    QVector<ImportTilesArea> areas;

    double zoom, scaling;
    QColor color_key;
    bool magnetic;

    int selected;
    QImage displayed_texture;

    QPoint mouse_cursor;
    std::optional<QPoint> click_origin;
    std::optional<QPoint> move_offset;

    void updateDisplayedTexture();

    QRect getSelectionRect() const;

    void drawBackground(QPainter &painter);
    void drawRectangles(QPainter &painter);
    void drawGrid(QPainter &painter);
    void drawSnapPoints(QPainter &painter, const int unit);
    void drawSelectionRect(QPainter &painter);
    void paintEvent(QPaintEvent *) final override;

    void mouseMoveEvent(QMouseEvent *event) final override;
    void mousePressEvent(QMouseEvent *event) final override;
    void mouseReleaseEvent(QMouseEvent *event) final override;
};

class ImportTilesInBulkDialog: public QDialog
{
    Q_OBJECT
public:
    explicit ImportTilesInBulkDialog(const int tilesize, QWidget *parent = nullptr);
    ~ImportTilesInBulkDialog();

    QString getTexturePath() const;
    QVector<ImportTilesArea> getTileAreas() const;

    QVector<SimpleTile> getTiles() const;

public slots:
    void onChangeTexturePath();
    void onChangeColorKey();

    void onAccept();

    void enableAreasWidgets(const bool enabled);
    void onAddArea(const ImportTilesArea area = {0, 0, 1, 1, 0, 0});
    void onRemoveArea(int index = -1);

    void enableAreaWidgets(const bool enabled);
    void onSelectedAreaChanged(const int index);
//  QListWidget::setCurrentRow is not a slot ; this one is merely doing the call
    void onSelectArea(const int index);

//  only transmitting the area changes to the rectangles view widget
    void onChangeArea();
//  only transmitting the changes to the x and y spinboxes
    void onAreaChanged(const QPoint new_position);

private:
    Ui::ImportTilesInBulkDialog *ui;

    int tilesize;
};
