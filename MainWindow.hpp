#pragma once

#include <QMainWindow>
#include <QUndoStack>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

using Names = QVector<QString>;
using Tileset = QHash<QString, QImage>;
using SelectedTiles = QVector<QVector<QString>>;

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
    void onQuit();

    void onUndo();
    void onRedo();

private:
    Ui::MainWindow *ui;

    QString save_path;

//  shared access to relevant child widgets -> make pointers
    QSharedPointer<QUndoStack> undo_stack;
    QSharedPointer<Names> tiles_order;
    QSharedPointer<Tileset> tileset;
    QSharedPointer<SelectedTiles> selected_tiles;

//  the user could choose cancel or save could fail => returns bool
//  true -> caller shall keep proceeding
//  false -> caller shall interrupt the routine
    bool handleUnsavedChanges();

//  centralises undo/redo changes
    void refreshViews();

    void closeEvent(QCloseEvent *event) override;
};
