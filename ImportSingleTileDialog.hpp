#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ImportSingleTileDialog; }
QT_END_NAMESPACE

using Tile = QVector<QImage>;

class ImportSingleTileWidget final: public QWidget
{
    Q_OBJECT
public:
    ImportSingleTileWidget(QWidget *parent = nullptr);

    void setTexture(const QString &path);
    void setTilesize(const int size) { tilesize = size; update(); }
    QVector<QPoint> getFrames() const { return frames; }

    void addFrame(const QPoint &frame) { frames.push_back(frame); update(); }
    void removeFrameAt(const int index);
    QPoint &getFrameAt(const int index);

    Tile getTile() const;

public slots:
    void setZoom(const double z) { zoom = z; updateDisplayedTexture(); }
    void setScaling(const double s) { scaling = s; updateDisplayedTexture(); }
    void setColorKey(const QColor color) { color_key = color; updateDisplayedTexture(); }
    void setMagnetic(const bool yes) { magnetic = yes; update(); }

    void setSelectedFrame(const int index) { selected = index; update(); }

signals:
    void frameSelected(const int index);
    void frameAdded(const QPoint frame);
    void frameRemoved(const int index);
    void frameChanged(const QPoint new_position);

private:
    int tilesize;
    QImage original_texture;
    QVector<QPoint> frames;

    double zoom, scaling;
    QColor color_key;
    bool magnetic;

    int selected;
    QImage displayed_texture;

    QPoint mouse_cursor;
    std::optional<QPoint> click_origin;
    std::optional<QPoint> move_offset;

    void updateDisplayedTexture();

    void drawBackground(QPainter &painter);
    void drawFrames(QPainter &painter);
    void drawFutureFrame(QPainter &painter);
    void drawGrid(QPainter &painter);
    void drawSnapPoints(QPainter &painter, const int unit);
    void paintEvent(QPaintEvent *) final override;

    void mouseMoveEvent(QMouseEvent *event) final override;
    void mousePressEvent(QMouseEvent *event) final override;
    void mouseReleaseEvent(QMouseEvent *event) final override;
};

class ImportSingleTileDialog: public QDialog
{
    Q_OBJECT
public:
    explicit ImportSingleTileDialog(const int tilesize, QWidget *parent = nullptr);
    ~ImportSingleTileDialog();

    QString getTexturePath() const;
    QVector<QPoint> getFrames() const;

    Tile getTile() const;

public slots:
    void onChangeTexturePath();
    void onChangeColorKey();

    void onAccept();

    void enableFramesWidgets(const bool enabled);
    void onAddFrame(const QPoint frame = {0, 0});
    void onRemoveFrame(int index = -1);

    void enableFrameWidgets(const bool enabled);
    void onSelectedFrameChanged(const int index);
//  QListWidget::setCurrentRow is not a slot ; this one is merely doing the call
    void onSelectFrame(const int index);

//  only transmitting the area changes to the rectangles view widget
    void onChangeFrame();
//  only transmitting the changes to the x and y spinboxes
    void onFrameChanged(const QPoint new_position);

private:
    Ui::ImportSingleTileDialog *ui;

    int tilesize;
};
