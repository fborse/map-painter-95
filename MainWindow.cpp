#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QClipboard>
#include <QUuid>

#include "NewMapDialog.hpp"

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent), ui(new Ui::MainWindow),
    save_path{},
    undo_stack{nullptr}, tiles_order{nullptr}, tileset{nullptr}
{
    ui->setupUi(this);

    EditorWidget *editors[] = {ui->mapEditor, ui->tilesetView, ui->mapPainter};

    undo_stack = QSharedPointer<QUndoStack>::create();
    for (auto *editor: editors)
        editor->setUndoStackPointer(undo_stack);
    tiles_order = QSharedPointer<Names>::create();
    for (auto *editor: editors)
        editor->setTilesOrderPointer(tiles_order);
    tileset = QSharedPointer<Tileset>::create();
    for (auto *editor: editors)
        editor->setTilesetPointer(tileset);
    selected_tiles = QSharedPointer<SelectedTiles>::create();
    for (auto *editor: editors)
        editor->setSelectedTilesPointer(selected_tiles);
    map_layers = QSharedPointer<MapLayer>::create();
    for (auto *editor: editors)
        editor->setMapLayersPointer(map_layers);

    {
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

    refreshViews();

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
}

MainWindow::~MainWindow()
{
    delete ui;
}

static inline void update_hue_widget(ColorHueWidget *widget, const QColor &color)
{
    const int hue = color.hsvHue();
    const int h = widget->height();

    widget->setHueHeight(hue * h / 360);
}

static inline void update_saturation_value_widget(ColorSaturationValueWidget *widget, const QColor &color)
{
    const int hue = color.hsvHue();
    const int saturation = color.hsvSaturation();
    const int value = color.value();
    const auto &[w, h] = widget->size();

    widget->setHue(hue);
    widget->setSaturation(saturation * w / 255);
    widget->setValue(h - (value * h / 255));
}

void MainWindow::updateColorWidgets(const QColor color)
{
    update_hue_widget(ui->hueWidget, color);
    update_saturation_value_widget(ui->saturationValueWidget, color);
    ui->alphaSpinBox->setValue(color.alpha());
}

void MainWindow::resetPointers()
{
    undo_stack->clear();
    tiles_order->clear();
    tileset->clear();
    selected_tiles->clear();
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
    QImage img(tilesize, tilesize, QImage::Format_ARGB32_Premultiplied);
    QColor colors[] = {{0, 128, 255}, {0, 128, 0}, {128, 64, 0}};

    for (auto &color: colors)
    {
        const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        tiles_order->push_back(id);

        img.fill(color);
        tileset->insert(id, img);
    }
}

void MainWindow::setMapSize(const QSize size)
{
    map_layers->resize(size.height());
    for (auto &row: *map_layers)
        row.resize(size.width());
}

bool MainWindow::handleUnsavedChanges()
{
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
            setMapSize(dialog.getMapSize());

            if (dialog.shouldImportTileset())
            {
                const char *text = "Not implemented yet !";
                QMessageBox::information(this, tr(text), tr(text));
            }
            else
            {
                setTilesize(dialog.getTilesize());
                if (dialog.shouldCreateDefaultTiles())
                    populateTileset(dialog.getTilesize());
            }

            resetBrushPixels();
            ui->mapPainter->resetSelection();
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

void MainWindow::onQuit()
{
    if (handleUnsavedChanges())
    {
        undo_stack->clear();
        qApp->exit(EXIT_SUCCESS);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (handleUnsavedChanges())
    {
        undo_stack->clear();
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::refreshViews()
{
    ui->mapEditor->resize();
    ui->tilesetView->resize();
    ui->mapPainter->resize();

//  selected_tiles is assumed to be small
    bool reset = false;
    for (auto &row: *selected_tiles)
        for (auto &id: row)
            if (!tileset->contains(id))
                reset = true;
    if (reset)
        *selected_tiles = {{{}}};   //  {{empty tile}}
}

void MainWindow::updateDrawOptions(const int draw_tool)
{
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
    undo_stack->undo();
    refreshViews();
}

void MainWindow::onRedo()
{
    undo_stack->redo();
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
{}

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
    Tileset tiles;
    for (int i = 0; i < n_tiles; ++i)
    {
        TileReference id;
        QImage tile;

        stream >> id >> tile;
        order.push_back(id);
        tiles[id] = tile;
    }

    MapLayer layers;
    stream >> layers;

    resetPointers();
    *tiles_order = std::move(order);
    *tileset = std::move(tiles);
    *map_layers = std::move(layers);

    setTilesize(tilesize);
    ui->tilesetView->setNumberOfColumns(n_columns);

    resetBrushPixels();
    ui->mapPainter->resetSelection();
    refreshViews();
    return true;
}
catch (const QString &errstr)
{
    const char *title = "Could not load the map !";
    QMessageBox::warning(this, tr(title), tr(errstr.toStdString().c_str()));

    return false;
}

bool MainWindow::save(const QString &path)
{
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
        const int major = 0, minor = 1;
        stream << major << minor;
    }

    {
        const int tilesize = ui->tilesetView->getTilesize();
        const int n_columns = ui->tilesetView->getNumberOfColumns();
        const int n_tiles = tiles_order->length();

        stream << tilesize << n_columns << n_tiles;

        for (auto &id: *tiles_order)
            stream << id << tileset->value(id);
    }

    {
        stream << *map_layers;
    }

    return true;
}
