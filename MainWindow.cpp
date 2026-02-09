#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QUuid>

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
        map_layers->resize(16);
        for (auto &row: *map_layers)
            row.resize(20, "");

        {
            QImage img(32, 32, QImage::Format_ARGB32_Premultiplied);
            QColor colors[] = {{0, 128, 255}, {0, 128, 0}, {128, 64, 0}};

            for (auto &color: colors)
            {
                const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
                tiles_order->push_back(id);

                img.fill(color);
                tileset->insert(id, img);
            }
        }
    }

    connect(undo_stack.get(), &QUndoStack::cleanChanged, ui->actionSave, &QAction::setDisabled);
    connect(undo_stack.get(), &QUndoStack::canUndoChanged, ui->actionUndo, &QAction::setEnabled);
    connect(undo_stack.get(), &QUndoStack::canRedoChanged, ui->actionRedo, &QAction::setEnabled);

    ui->actionSave->setEnabled(false);
    ui->actionUndo->setEnabled(false);
    ui->actionRedo->setEnabled(false);

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
}

MainWindow::~MainWindow()
{
    delete ui;
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
    {}
}

void MainWindow::onOpen()
{
    if (handleUnsavedChanges())
    {}
}

bool MainWindow::onSave()
{
    if (save_path.isEmpty())
    {
        return onSaveAs();
    }
    else
    {
        undo_stack->setClean();
        return true;
    }
}

bool MainWindow::onSaveAs()
{
    const QString path = QFileDialog::getSaveFileName(this);
    if (path.isEmpty())
        return false;

    save_path = path;

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
