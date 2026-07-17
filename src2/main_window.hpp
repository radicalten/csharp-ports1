#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QTreeWidget>
#include <QAction>
#include <filesystem>

#include "document_manager.hpp"

namespace fs = std::filesystem;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    dsm::DocumentManager mgr;
    QTreeWidget* tree = nullptr;
    QTextEdit* log = nullptr;
    QProgressBar* progress = nullptr;
    QLabel* status = nullptr;
    QLabel* modified = nullptr;
    QTreeWidgetItem* root_item = nullptr;
    QAction* undo_act = nullptr;
    QAction* redo_act = nullptr;
    QAction* sort_act = nullptr;
    QAction* sort_rec_act = nullptr;
    QAction* save_act = nullptr;

    void build_menu();
    void refresh_tree();
    void add_node(QTreeWidgetItem* parent, const dsm::ArchiveEntryPtr& e);
    void on_open();
    void on_save();
    void on_save_as();
    void on_new();
    void on_context_menu(const QPoint& pos);
    void on_rename(const dsm::ArchiveEntryPtr& e);
    void on_import_folder();
    QString pick_dir();
    void update_modified();
    void update_edit_menu();
    void log_msg(const std::string& m);
};
