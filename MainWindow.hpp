#pragma once

#include <QMainWindow>
#include <QUndoStack>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

using Names = QVector<QString>;
using Tile = QVector<QImage>;
using Tileset = QHash<QString, Tile>;
using SelectedTiles = QVector<QVector<QString>>;

using TileReference = QString;
using MapLayer = QVector<QVector<TileReference>>;
using MapLayers = QVector<MapLayer>;

class MainWindow: public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void onNew();
    void onOpen();
//  see handleUnsavedChanges() for why bool
    bool onSave();
    bool onSaveAs();
    void onImportSingleTile();
    void onImportTilesInBulk();
    void onExportTilesetAndMap();
    void onExportAsTextures();
    void onQuit();

    void onUndo();
    void onRedo();
    void onCut();
    void onCopy();
    void onPaste();
    void onSelectAll();

    void onAddTile();
    void onCloneSelectedTiles();
    void onRemoveSelectedTiles();
    void onAddFrame();
    void onCloneCurrentFrame();
    void onRemoveCurrentFrame();

    void onResizeMap();
    void onAddLayer();
    void onRemoveCurrentLayer();

    void onFlipHorizontally();
    void onFlipVertically();
    void onRotate90CW();
    void onRotate90CCW();
    void onScale();

    void onSelectedChanged();

//  centralises undo/redo changes
    void refreshViews();

    void updateLayersBoxes();
    void updateFramesBoxes();

    void updateColorWidgets(const QColor color);
    void updateDrawOptions(const int draw_tool);

    void onMapWidgetScrolled(const QPoint p);

private:
    Ui::MainWindow *ui;

    QString save_path;

//  shared access to relevant child widgets -> make pointers
    QSharedPointer<QUndoStack> undo_stack;
    QSharedPointer<Names> tiles_order;
    QSharedPointer<Tileset> tileset;
    QSharedPointer<SelectedTiles> selected_tiles;
    QSharedPointer<MapLayers> map_layers;

    void resetPointers();
    void resetBrushPixels();
    void setTilesize(const int tilesize);
    void populateTileset(const int tilesize);
    void setMapSize(const QSize size);

//  the user could choose cancel or save could fail => returns bool
//  true -> caller shall keep proceeding
//  false -> caller shall interrupt the routine
    bool handleUnsavedChanges();

    void closeEvent(QCloseEvent *event) override;

    bool load(const QString &path);
    bool save(const QString &path);
};
