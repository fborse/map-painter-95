#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QClipboard>
#include <QScrollBar>
#include <QUuid>

#include "NewMapDialog.hpp"
#include "AddRectDialog.hpp"
#include "ResizeMapDialog.hpp"
#include "ScaleSelectionDialog.hpp"
#include "ImportSingleTileDialog.hpp"
#include "ImportTilesInBulkDialog.hpp"
#include "ExportTilesetAndMapDialog.hpp"
#include "ExportAsTexturesDialog.hpp"

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent), ui(new Ui::MainWindow),
    save_path{},
    undo_stack{nullptr},
    simple_tiles_order{nullptr}, autotiles_order{nullptr},
    simple_tiles{nullptr}, autotiles{nullptr},
    selected_tiles{nullptr}, map_layers{nullptr}
{
    ui->setupUi(this);

    EditorWidget *editors[] = {ui->mapEditor, ui->tilesetView, ui->mapPainter};

    undo_stack = QSharedPointer<QUndoStack>::create();
    for (auto *editor: editors)
        editor->setUndoStackPointer(undo_stack);
    simple_tiles_order = QSharedPointer<Names>::create();
    for (auto *editor: editors)
        editor->setSimpleTilesOrderPointer(simple_tiles_order);
    autotiles_order = QSharedPointer<Names>::create();
    for (auto *editor: editors)
        editor->setAutoTilesOrderPointer(autotiles_order);
    simple_tiles = QSharedPointer<SimpleTiles>::create();
    for (auto *editor: editors)
        editor->setSimpleTilesPointer(simple_tiles);
    autotiles = QSharedPointer<AutoTiles>::create();
    for (auto *editor: editors)
        editor->setAutoTilesPointer(autotiles);
    selected_tiles = QSharedPointer<SelectedTiles>::create();
    for (auto *editor: editors)
        editor->setSelectedTilesPointer(selected_tiles);
    map_layers = QSharedPointer<MapLayers>::create();
    for (auto *editor: editors)
        editor->setMapLayersPointer(map_layers);

    {
        map_layers->resize(1);
        setMapSize({20, 16});

        populateTileset(32);
        resetBrushPixels();
    }

    connect(undo_stack.get(), &QUndoStack::cleanChanged, ui->actionSave, &QAction::setDisabled);
    connect(undo_stack.get(), &QUndoStack::canUndoChanged, ui->actionUndo, &QAction::setEnabled);
    connect(undo_stack.get(), &QUndoStack::canRedoChanged, ui->actionRedo, &QAction::setEnabled);

    ui->actionSave->setEnabled(false);
    ui->actionUndo->setEnabled(false);
    ui->actionRedo->setEnabled(false);

    ui->actionCut->setEnabled(false);
    ui->actionCopy->setEnabled(false);
    ui->actionFlipHorizontally->setEnabled(false);
    ui->actionFlipVertically->setEnabled(false);
    ui->actionRotate90CW->setEnabled(false);
    ui->actionRotate90CCW->setEnabled(false);
    ui->actionScale->setEnabled(false);

    ui->actionCloneSelectedTiles->setEnabled(false);
    ui->actionAddFrame->setEnabled(false);
    ui->actionCloneCurrentFrame->setEnabled(false);
    ui->actionRemoveSelectedTiles->setEnabled(false);
    ui->actionRemoveCurrentFrame->setEnabled(false);

    refreshViews();
    updateLayersBoxes();
    updateFramesBoxes();

    ui->leftColorWidget->setColor(Qt::black);
    connect(ui->leftColorWidget, &ColorSelectionWidget::clicked, [&] {
        ui->leftColorWidget->setColor(ui->mapPainter->getDrawColor());
        ui->colorGradientWidget->setLeftColor(ui->mapPainter->getDrawColor());
    });
    ui->rightColorWidget->setColor(Qt::white);
    connect(ui->rightColorWidget, &ColorSelectionWidget::clicked, [&] {
        ui->rightColorWidget->setColor(ui->mapPainter->getDrawColor());
        ui->colorGradientWidget->setRightColor(ui->mapPainter->getDrawColor());
    });

    connect(ui->shapeComboBox, &QComboBox::currentIndexChanged, [&] (const int index) {
        ui->cornerRadiusSpinBox->setEnabled(index == 0);
    });

    {
        QScrollBar *h1 = ui->mapViewScrollArea->horizontalScrollBar();
        QScrollBar *h2 = ui->mapPainterScrollArea->horizontalScrollBar();
        connect(h1, &QScrollBar::valueChanged, h2, &QScrollBar::setValue);
        connect(h2, &QScrollBar::valueChanged, h1, &QScrollBar::setValue);

        QScrollBar *v1 = ui->mapViewScrollArea->verticalScrollBar();
        QScrollBar *v2 = ui->mapPainterScrollArea->verticalScrollBar();
        connect(v1, &QScrollBar::valueChanged, v2, &QScrollBar::setValue);
        connect(v2, &QScrollBar::valueChanged, v1, &QScrollBar::setValue);
    }

//  TODO: is there *really* no better way to set this kind of stuff ???
    ui->splitter->setStretchFactor(0, 3);
    ui->splitter->setStretchFactor(1, 1);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onMapWidgetScrolled(const QPoint p)
{
    const auto &[x, y] = p.toPointF();
    const auto &[w, h] = ui->mapEditor->size();

    QScrollBar *he = ui->mapViewScrollArea->horizontalScrollBar();
    QScrollBar *ve = ui->mapViewScrollArea->verticalScrollBar();

    he->setValue(x * he->maximum() / w);
    ve->setValue(y * ve->maximum() / h);
}

void MainWindow::updateColorWidgets(const QColor color)
{
    ui->hueWidget->setHue(color.hsvHueF());

    ui->saturationValueWidget->setHue(color.hsvHueF());
    ui->saturationValueWidget->setSaturation(color.hsvSaturationF());
    ui->saturationValueWidget->setValue(color.valueF());

    ui->alphaSpinBox->setValue(color.alpha());
}

void MainWindow::resetPointers()
{
    Q_ASSERT(!undo_stack.isNull());
    undo_stack->clear();
    Q_ASSERT(!simple_tiles_order.isNull());
    simple_tiles_order->clear();
    Q_ASSERT(!simple_tiles.isNull());
    simple_tiles->clear();
    Q_ASSERT(!selected_tiles.isNull());
    selected_tiles->clear();
    Q_ASSERT(!map_layers.isNull());
    map_layers->clear();
}

void MainWindow::resetBrushPixels()
{
    QImage pixels(16, 16, QImage::Format_ARGB32_Premultiplied);
    pixels.fill(Qt::black);
    pixels.setPixelColor(7, 7, Qt::white);
    pixels.setPixelColor(7, 8, Qt::white);
    pixels.setPixelColor(8, 7, Qt::white);
    pixels.setPixelColor(8, 8, Qt::white);

    ui->brushDisplayWidget->setBrushPixels(pixels);
    ui->mapPainter->setBrushPixels(pixels);
}

void MainWindow::setTilesize(const int tilesize)
{
    ui->tilesetView->setTilesize(tilesize);
    ui->mapEditor->setTilesize(tilesize);
    ui->mapPainter->setTilesize(tilesize);
}

void MainWindow::populateTileset(const int tilesize)
{
    Q_ASSERT(!simple_tiles_order.isNull());
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(tilesize > 0);

    QImage pixels(tilesize, tilesize, QImage::Format_ARGB32_Premultiplied);
    const QColor colors[] = {{0, 128, 255}, {0, 128, 0}, {128, 64, 0}};

    QVector<SimpleTile> tiles;
    for (auto &color: colors)
    {
        pixels.fill(color);
        tiles.push_back({{pixels}});
    }

    ui->tilesetView->addSimpleTiles({tiles}, false);
}

void MainWindow::setMapSize(const QSize size)
{
    Q_ASSERT(!map_layers.isNull());
    Q_ASSERT(size.width() > 0 && size.height() > 0);

    for (auto &layer: *map_layers)
    {
        layer.resize(size.height());

        for (auto &row: layer)
            row.resize(size.width());
    }

    ui->mapEditor->setGridAspect(size);
    ui->mapPainter->setGridAspect(size);
}

bool MainWindow::handleUnsavedChanges()
{
    Q_ASSERT(!undo_stack.isNull());
    if (undo_stack->isClean())
        return true;

    const char *title = "There are unsaved changes !";
    const char *msg = "Would you like to save the changes ?";
    const auto buttons = QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel;
    const auto defaut = QMessageBox::Save;

    const auto answer = QMessageBox::question(this, tr(title), tr(msg), buttons, defaut);
    switch (answer)
    {
    case QMessageBox::Save:
        return onSave();
    case QMessageBox::Discard:
        return true;
    case QMessageBox::Cancel:
    default:    //  saving failed or user pressed escape
        return false;
    }
}

void MainWindow::onNew()
{
    if (handleUnsavedChanges())
    {
        NewMapDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted)
        {
            resetPointers();
            Q_ASSERT(!map_layers.isNull());
            map_layers->resize(1);
            Q_ASSERT(dialog.getMapSize().width() > 0);
            Q_ASSERT(dialog.getMapSize().height() > 0);
            setMapSize(dialog.getMapSize());

            if (dialog.shouldImportTileset())
            {
                const char *text = "Not implemented yet !";
                QMessageBox::information(this, tr(text), tr(text));
            }
            else
            {
                Q_ASSERT(dialog.getTilesize() > 0);
                setTilesize(dialog.getTilesize());
                if (dialog.shouldCreateDefaultTiles())
                    populateTileset(dialog.getTilesize());
            }

            resetBrushPixels();
            ui->mapPainter->resetSelection();
            updateLayersBoxes();
            updateFramesBoxes();
            refreshViews();
        }
    }
}

void MainWindow::onOpen()
{
    if (handleUnsavedChanges())
    {
        const char *title = "Choose a Map Painter 95 file to open !";
        const char *ext = "Map Painter 95 files (*.mp95)";
        const QString path = QFileDialog::getOpenFileName(this, tr(title), QString(), tr(ext));

        if (!path.isEmpty())
            if (load(path))
                save_path = path;
    }
}

bool MainWindow::onSave()
{
    Q_ASSERT(!undo_stack.isNull());

    if (save_path.isEmpty())
    {
        return onSaveAs();
    }
    else
    {
        if (!save(save_path))
            return false;

        undo_stack->setClean();
        return true;
    }
}

bool MainWindow::onSaveAs()
{
    Q_ASSERT(!undo_stack.isNull());

    const char *title = "Choose a Map Painter 95 file to save to !";
    const char *ext = "Map Painter 95 files (*.mp95)";
    const QString path = QFileDialog::getSaveFileName(this, tr(title), QString(), tr(ext));
    if (path.isEmpty())
        return false;

    if (!save(path))
        return false;

    save_path = path;
    undo_stack->setClean();
    return true;
}

void MainWindow::onImportSingleTile()
{
    const int tilesize = ui->tilesetView->getTilesize();

    ImportSingleTileDialog dialog(tilesize, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        ui->tilesetView->addSimpleTiles({dialog.getTile()}, true);
        refreshViews();
        updateFramesBoxes();
    }
}

void MainWindow::onImportTilesInBulk()
{
    const int tilesize = ui->tilesetView->getTilesize();

    ImportTilesInBulkDialog dialog(tilesize, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        ui->tilesetView->addSimpleTiles(dialog.getTiles(), true);
        refreshViews();
    }
}

void MainWindow::onExportTilesetAndMap()
{
    Q_ASSERT(!simple_tiles_order.isNull());
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(!map_layers.isNull());

    const int tilesize = ui->tilesetView->getTilesize();

    ExportTilesetAndMapDialog dialog(tilesize, this);
    dialog.setSimpleTilesOrderPointer(simple_tiles_order);
    dialog.setSimpleTilesPointer(simple_tiles);
    dialog.setMapLayersPointer(map_layers);
    dialog.exec();
}

void MainWindow::onExportAsTextures()
{
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(!map_layers.isNull());

    const int tilesize = ui->tilesetView->getTilesize();

    ExportAsTexturesDialog dialog(tilesize, this);
    dialog.setSimpleTilesPointer(simple_tiles);
    dialog.setMapLayersPointer(map_layers);
    dialog.exec();
}

void MainWindow::onQuit()
{
    if (handleUnsavedChanges())
    {
        Q_ASSERT(!undo_stack.isNull());
        undo_stack->clear();
        qApp->exit(EXIT_SUCCESS);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (handleUnsavedChanges())
    {
        Q_ASSERT(!undo_stack.isNull());
        undo_stack->clear();
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

static inline bool not_empty(const SelectedTiles &tiles)
{
    return (tiles.length() > 0) && (tiles.at(0).length() > 0);
}

static inline bool is_1x1(const SelectedTiles &tiles)
{
    return (tiles.length() == 1) && (tiles.at(0).length() == 1);
}

static inline bool can_add_frames(const SimpleTiles &simple_tiles, const SelectedTiles &tiles)
{
    if (!is_1x1(tiles))
        return false;
    else
        return simple_tiles.contains(tiles[0][0].name);
}

static inline bool can_remove_frames(const SimpleTiles &simple_tiles, const SelectedTiles &tiles)
{
    if (!is_1x1(tiles))
        return false;
    else if (!simple_tiles.contains(tiles[0][0].name))
        return false;
    else
        return (simple_tiles.value(tiles[0][0].name).frames.length() > 1);
}

void MainWindow::onSelectedChanged()
{
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(!selected_tiles.isNull());

    ui->tilesetView->update();

    ui->actionCloneSelectedTiles->setEnabled(not_empty(*selected_tiles));
    ui->actionRemoveSelectedTiles->setEnabled(not_empty(*selected_tiles));

    ui->actionAddFrame->setEnabled(can_add_frames(*simple_tiles, *selected_tiles));
    ui->actionCloneCurrentFrame->setEnabled(can_add_frames(*simple_tiles, *selected_tiles));
    ui->actionRemoveCurrentFrame->setEnabled(can_remove_frames(*simple_tiles, *selected_tiles));
}

void MainWindow::refreshViews()
{
    ui->mapEditor->resize();
    ui->tilesetView->resize();
    ui->mapPainter->resize();

    Q_ASSERT(!selected_tiles.isNull());
//  selected_tiles is assumed to be small
    bool reset = false;
    for (auto &row: *selected_tiles)
        for (auto &ref: row)
            if (!(simple_tiles->contains(ref.name) || autotiles->contains(ref.name)))
                reset = true;
    if (reset)
        *selected_tiles = {{{}}};   //  {{empty tile}}
}

static inline void resize_cb(QComboBox *box, const int n)
{
    const int current = box->currentIndex();

    box->clear();
    for (int i = 0; i < n; ++i)
        box->addItem(QString::number(i + 1));

    if (current < n)
        box->setCurrentIndex(current);
    else
        box->setCurrentIndex(n - 1);
}

void MainWindow::updateLayersBoxes()
{
    Q_ASSERT(!map_layers.isNull());
    const int n = map_layers->length();

    resize_cb(ui->currentLayerMapViewComboBox, n);
    resize_cb(ui->currentLayerMapPainterComboBox, n);

    ui->actionRemoveLayer->setEnabled(n > 1);
}

static inline int get_max_frames(const SimpleTiles &simple_tiles)
{
    int max = 0;

    for (auto &tile: simple_tiles.values())
        if (max < tile.frames.length())
            max = tile.frames.length();

    return max;
}

void MainWindow::updateFramesBoxes()
{
    Q_ASSERT(!simple_tiles.isNull());
    const int n = get_max_frames(*simple_tiles);

    resize_cb(ui->currentFrameMapViewComboBox, n);
    resize_cb(ui->currentFrameMapPainterComboBox, n);
}

void MainWindow::updateDrawOptions(const int draw_tool)
{
//  TODO: find a way to remove the magic aspect of this magic value
    const int n = 9;
    Q_ASSERT(0 <= draw_tool && draw_tool < n);

//  Stack 1 : Empty page, Pen page, Fill page, Selection page
//  Stack 2 : Empty page, Brush page, Shape page, Shader page

//  pen, line, brush, shape, fill, eraser, shader, pipette, selection
    int stack1[n] = {1, 1, 1, 1, 2, 1, 1, 0, 3};
    int stack2[n] = {0, 0, 1, 2, 0, 0, 3, 0, 0};

    ui->toolBox1StackedWidget->setCurrentIndex(stack1[draw_tool]);
    ui->toolBox2StackedWidget->setCurrentIndex(stack2[draw_tool]);
}

void MainWindow::onUndo()
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(undo_stack->canUndo());

    undo_stack->undo();
    updateLayersBoxes();
    updateFramesBoxes();
    refreshViews();
}

void MainWindow::onRedo()
{
    Q_ASSERT(!undo_stack.isNull());
    Q_ASSERT(undo_stack->canRedo());

    undo_stack->redo();
    updateLayersBoxes();
    updateFramesBoxes();
    refreshViews();
}

void MainWindow::onCut()
{
    QApplication::clipboard()->setImage(ui->mapPainter->getSelectionImage());
    ui->mapPainter->cutSelection();
}

void MainWindow::onCopy()
{
    QApplication::clipboard()->setImage(ui->mapPainter->getSelectionImage());
}

void MainWindow::onPaste()
{
    const QImage copy = QApplication::clipboard()->image();
    if (!copy.isNull())
    {
        ui->mainTabWidget->setCurrentIndex(1);
        ui->drawToolComboBox->setCurrentIndex(int(SELECTION));

        ui->mapPainter->setSelectionImage(copy);
        ui->mapPainter->setSelectionRect({0, 0, copy.width(), copy.height()});
        ui->mapPainter->update();
    }
}

void MainWindow::onSelectAll()
{
    ui->mainTabWidget->setCurrentIndex(1);
    ui->drawToolComboBox->setCurrentIndex(int(SELECTION));
    ui->selectionShapeComboBox->setCurrentIndex(int(RECTANGLE));

    ui->mapPainter->selectAll();
}

static inline QImage query_tile(const int tilesize, const QString &title, QWidget *parent)
{
    Q_ASSERT(tilesize > 0);
    QImage tile;

    AddRectDialog dialog(tilesize, parent);
    dialog.setWindowTitle(title);
    if (dialog.exec() == QDialog::Accepted)
    {
        tile = QImage(tilesize, tilesize, QImage::Format_ARGB32_Premultiplied);
        tile.fill(dialog.getColor());
    }

    return tile;
}

void MainWindow::onAddSimpleTile()
{
    const int tilesize = ui->tilesetView->getTilesize();
    QImage tile = query_tile(tilesize, "Add Simple Tile", this);

    if (!tile.isNull())
        ui->tilesetView->addSimpleTiles({SimpleTile{{tile}}}, true);
}

void MainWindow::onAddAutoTile()
{
    const int tilesize = ui->tilesetView->getTilesize();
    QImage metatile = query_tile(tilesize/2, "Add Autotile", this);

//  see Types.hpp for why
    const size_t n = 20;

    if (!metatile.isNull())
    {
        AutoTile tile;
        tile.frames.push_back({QVector<QImage>(n, metatile)});

        ui->tilesetView->addAutoTile(tile, true);
    }
}

static inline QSet<TileReference> get_unique_valid_ids(const SimpleTiles &simple_tiles, const SelectedTiles &selected)
{
    QSet<TileReference> uniques;

    for (auto &row: selected)
        for (auto &ref: row)
            if (simple_tiles.contains(ref.name))
                uniques.insert(ref);

    return uniques;
}

void MainWindow::onCloneSelectedTiles()
{
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(!selected_tiles.isNull());

    QSet<TileReference> uniques = get_unique_valid_ids(*simple_tiles, *selected_tiles);
    if (uniques.isEmpty())
        return;

    QVector<SimpleTile> tiles;
    for (auto &ref: uniques)
    {
        Q_ASSERT(simple_tiles->contains(ref.name));
        tiles.push_back(simple_tiles->value(ref.name));
    }

    ui->tilesetView->addSimpleTiles(tiles, true);
}

void MainWindow::onRemoveSelectedTiles()
{
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(!selected_tiles.isNull());

    QSet<TileReference> uniques = get_unique_valid_ids(*simple_tiles, *selected_tiles);
    if (uniques.isEmpty())
        return;

    QVector<TileReference> tiles;
    for (auto &ref: uniques)
    {
        Q_ASSERT(simple_tiles->contains(ref.name));
        tiles.push_back(ref);
    }

    ui->tilesetView->removeTiles(tiles);
}

void MainWindow::onAddFrame()
{
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(!selected_tiles.isNull());
    Q_ASSERT(is_1x1(*selected_tiles));
    const auto ref = selected_tiles->at(0).at(0);
    Q_ASSERT(simple_tiles->contains(ref.name));

    const int tilesize = ui->tilesetView->getTilesize();
    Q_ASSERT(tilesize > 0);

    const int current_frame = ui->currentFrameMapViewComboBox->currentIndex();

    AddRectDialog dialog(tilesize, this);
    dialog.setWindowTitle("Add Frame");
    if (dialog.exec() == QDialog::Accepted)
    {
        QImage frame(tilesize, tilesize, QImage::Format_ARGB32_Premultiplied);
        frame.fill(dialog.getColor());

        ui->tilesetView->addFrames({{current_frame + 1, frame}});

        updateFramesBoxes();
        ui->actionRemoveCurrentFrame->setEnabled(can_remove_frames(*simple_tiles, *selected_tiles));
    }
}

void MainWindow::onCloneCurrentFrame()
{
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(!selected_tiles.isNull());
    Q_ASSERT(is_1x1(*selected_tiles));
    const auto ref = selected_tiles->at(0).at(0);
    Q_ASSERT(simple_tiles->contains(ref.name));

    const int current_frame = ui->currentFrameMapViewComboBox->currentIndex();
    auto &frames = (*simple_tiles)[ref.name].frames;
    const int n = frames.length();

    QImage frame = frames[qMin(current_frame, n)];

    ui->tilesetView->addFrames({{current_frame + 1, frame}});

    updateFramesBoxes();
    ui->actionRemoveCurrentFrame->setEnabled(can_remove_frames(*simple_tiles, *selected_tiles));
}

void MainWindow::onRemoveCurrentFrame()
{
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(!selected_tiles.isNull());
    Q_ASSERT(is_1x1(*selected_tiles));
    const auto ref = selected_tiles->at(0).at(0);
    Q_ASSERT(simple_tiles->contains(ref.name));

    const int current_frame = ui->currentFrameMapViewComboBox->currentIndex();

    ui->tilesetView->removeFrames({current_frame});

    updateFramesBoxes();
    ui->actionRemoveCurrentFrame->setEnabled(can_remove_frames(*simple_tiles, *selected_tiles));
}

void MainWindow::onResizeMap()
{
    Q_ASSERT(!map_layers.isNull());
    const int h = map_layers->length();
    Q_ASSERT(!map_layers->isEmpty());
    const int w = map_layers->at(0).length();
    Q_ASSERT(!map_layers->at(0).isEmpty());

    ResizeMapDialog dialog(this);
    dialog.setSize({w, h});
    if (dialog.exec() == QDialog::Accepted)
        ui->mapEditor->resizeMap(dialog.getSize());
}

void MainWindow::onAddLayer()
{
    Q_ASSERT(!map_layers.isNull());
    const int current_layer = ui->currentLayerMapViewComboBox->currentIndex();
    Q_ASSERT(0 <= current_layer);
    Q_ASSERT(current_layer < map_layers->length());

    ui->mapEditor->insertLayer(current_layer + 1);
    Q_ASSERT(current_layer + 1 < map_layers->length());
    updateLayersBoxes();
    ui->currentLayerMapViewComboBox->setCurrentIndex(current_layer + 1);
}

void MainWindow::onRemoveCurrentLayer()
{
    Q_ASSERT(!map_layers.isNull());
    const int current_layer = ui->currentLayerMapViewComboBox->currentIndex();
    Q_ASSERT(0 <= current_layer);
    Q_ASSERT(current_layer < map_layers->length());

    ui->mapEditor->removeLayer(current_layer);
    Q_ASSERT(current_layer - 1 < map_layers->length());
    if (current_layer == map_layers->length())
        ui->currentLayerMapViewComboBox->setCurrentIndex(current_layer - 1);
    updateLayersBoxes();
}

void MainWindow::onFlipHorizontally()
{
    ui->mapPainter->flipSelection(true, false);
}

void MainWindow::onFlipVertically()
{
    ui->mapPainter->flipSelection(false, true);
}

void MainWindow::onRotate90CW()
{
    ui->mapPainter->rotateSelection(-90);
}

void MainWindow::onRotate90CCW()
{
    ui->mapPainter->rotateSelection(90);
}

void MainWindow::onScale()
{
    const QSize original_size = ui->mapPainter->getSelectionImage().size();
    ScaleSelectionDialog dialog(original_size, this);

    if (dialog.exec() == QDialog::Accepted)
        ui->mapPainter->scaleSelection(dialog.getHorizontalFactor(), dialog.getVerticalFactor());
}

static inline QDataStream &operator>>(QDataStream &stream, Orientation &o)
{
    stream >> o.top_left >> o.top_right >> o.bottom_left >> o.bottom_right;

    return stream;
}

static inline QDataStream &operator>>(QDataStream &stream, TileReference &ref)
{
    stream >> ref.name >> ref.autotile >> ref.orientation;

    return stream;
}

bool MainWindow::load(const QString &path) try
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        throw QString("Could not read from '%1' !").arg(path);

    QDataStream stream(&file);

    int major, minor;
    stream >> major >> minor;

    int tilesize, n_columns, n_tiles;
    stream >> tilesize >> n_columns >> n_tiles;
    if (tilesize < 1 || tilesize > 256)
        throw QString("Invalid tilesize %1 !").arg(tilesize);
    if (n_columns < 1 || n_columns > 16)
        throw QString("Invalid number of columns %1 !").arg(n_columns);
    if (n_tiles < 0)
        throw QString("Invalid number of tiles %1 !").arg(n_tiles);

    Names order;
    SimpleTiles tiles;
    for (int i = 0; i < n_tiles; ++i)
    {
        TileReference ref;
        SimpleTile tile;

        stream >> ref.name >> tile.frames;
        order.push_back(ref.name);
        tiles[ref.name] = tile;
    }

    MapLayers layers;
    stream >> layers;

    resetPointers();
    Q_ASSERT(!simple_tiles_order.isNull());
    *simple_tiles_order = std::move(order);
    Q_ASSERT(!simple_tiles.isNull());
    *simple_tiles = std::move(tiles);
    Q_ASSERT(!map_layers.isNull());
    *map_layers = std::move(layers);

    setTilesize(tilesize);
    ui->tilesetView->setNumberOfColumns(n_columns);

    resetBrushPixels();
    ui->mapPainter->resetSelection();
    updateLayersBoxes();
    updateFramesBoxes();
    refreshViews();
    return true;
}
catch (const QString &errstr)
{
    const char *title = "Could not load the map !";
    QMessageBox::warning(this, tr(title), tr(errstr.toStdString().c_str()));

    return false;
}

static inline QDataStream &operator<<(QDataStream &stream, const Orientation &o)
{
    stream << o.top_left << o.top_right << o.bottom_left << o.bottom_right;

    return stream;
}

static inline QDataStream &operator<<(QDataStream &stream, const TileReference &ref)
{
    stream << ref.name << ref.autotile << ref.orientation;

    return stream;
}

bool MainWindow::save(const QString &path)
{
    Q_ASSERT(!simple_tiles_order.isNull());
    Q_ASSERT(!simple_tiles.isNull());
    Q_ASSERT(!map_layers.isNull());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        const char *title = "Could not save the map !";
        const auto msg = QString("Could not write to '%1' !").arg(path).toStdString();

        QMessageBox::warning(this, tr(title), tr(msg.c_str()));
        return false;
    }

    QDataStream stream(&file);

    {
        const int major = 1, minor = 0;
        stream << major << minor;
    }

    {
        const int tilesize = ui->tilesetView->getTilesize();
        const int n_columns = ui->tilesetView->getNumberOfColumns();
        const int n_tiles = simple_tiles_order->length();

        stream << tilesize << n_columns << n_tiles;

        for (auto &id: *simple_tiles_order)
            stream << id << simple_tiles->value(id).frames;
    }

    {
        stream << *map_layers;
    }

    return true;
}
