#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ImportTilesInBulkDialog; }
QT_END_NAMESPACE

class ImportTilesInBulkWidget final: public QWidget
{
    Q_OBJECT
public:
    ImportTilesInBulkWidget(QWidget *parent = nullptr);

    void setTexture(const QString &path);
    void setTilesize(const int size) { tilesize = size; update(); }
    QVector<QRect> getRectangles() const { return rectangles; }

    void addRectangle(const QRect &rect) { rectangles.push_back(rect); update(); }
    void removeRectangleAt(const int index);
    QRect &getRectangleAt(const int index);

public slots:
    void setZoom(const double z) { zoom = z; updateDisplayedTexture(); }
    void setScaling(const double s) { scaling = s; updateDisplayedTexture(); }
    void setColorKey(const QColor color) { color_key = color; updateDisplayedTexture(); }
    void setMagnetic(const bool yes) { magnetic = yes; update(); }

    void setSelectedRectangle(const int index) { selected = index; update(); }

signals:
    void rectangleSelected(const int index);
    void rectangleAdded(const QRect rectangle);
    void rectangleRemoved(const int index);
    void rectangleChanged(const QPoint new_position);

private:
    int tilesize;
    QImage original_texture;
    QVector<QRect> rectangles;

    double zoom, scaling;
    QColor color_key;
    bool magnetic;

    int selected;
    QImage displayed_texture;

    QPoint mouse_cursor;
    std::optional<QPoint> click_origin;
    std::optional<QPoint> move_offset;

    void updateDisplayedTexture();
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
    QVector<QRect> getTileAreas() const;

public slots:
    void onChangeTexturePath();
    void onChangeColorKey();

    void onAccept();

    void enableAreasWidgets(const bool enabled);
    void onAddArea(const QRect area = {0, 0, 1, 1});
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
