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

    void setSelectedArea(const int index) { selected = index; }

signals:
    void areaSelected(const int index);

private:
    int tilesize;
    QImage original_texture;
    QVector<QRect> rectangles;

    double zoom, scaling;
    QColor color_key;
    bool magnetic;

    int selected;
    QImage displayed_texture;

    void updateDisplayedTexture();
    void paintEvent(QPaintEvent *) final override;
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

    void onAddArea();
    void onRemoveArea();

    void onSelectedAreaChanged(const int index);

private:
    Ui::ImportTilesInBulkDialog *ui;

    int tilesize;
};
