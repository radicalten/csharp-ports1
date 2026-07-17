#include "main_window.hpp"
#include "dialog_service.hpp"
#include <QMenuBar>
#include <QTreeWidget>
#include <QFileDialog>
#include <QProgressBar>
#include <QTextEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>
#include <QTime>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Disgaea DS Manager");
    resize(1000, 700);

    tree = new QTreeWidget(this);
    tree->setHeaderHidden(true);
    log = new QTextEdit(this);
    log->setReadOnly(true);
    progress = new QProgressBar(this); progress->setMaximum(100);
    status = new QLabel("Ready");
    modified = new QLabel("");

    auto* split = new QSplitter(Qt::Vertical, this);
    split->addWidget(tree);
    split->addWidget(log);
    split->setStretchFactor(0, 2); split->setStretchFactor(1, 1);

    auto* bottom = new QWidget(this);
    auto* bl = new QHBoxLayout(bottom);
    bl->addWidget(status); bl->addWidget(modified); bl->addWidget(progress);
    bottom->setLayout(bl);

    auto* central = new QWidget(this);
    auto* cl = new QVBoxLayout(central);
    cl->addWidget(split); cl->addWidget(bottom);
    setCentralWidget(central);

    build_menu();
    mgr.document_changed = [this]() { refresh_tree(); };

    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tree, &QTreeWidget::customContextMenuRequested, this, &MainWindow::on_context_menu);
    connect(tree, &QTreeWidget::itemSelectionChanged, this, &MainWindow::update_edit_menu);
}

void MainWindow::build_menu() {
    auto* file = menuBar()->addMenu("&File");
    file->addAction("&New...", Qt::CTRL + Qt::Key_N, [this]() { on_new(); });
    file->addAction("&Open...", Qt::CTRL + Qt::Key_O, [this]() { on_open(); });
    file->addSeparator();
    save_act = file->addAction("&Save", Qt::CTRL + Qt::Key_S, [this]() { on_save(); });
    file->addAction("Save &As...", Qt::CTRL + Qt::SHIFT + Qt::Key_S, [this]() { on_save_as(); });
    file->addSeparator();
    file->addAction("E&xit", [this]() { close(); });

    auto* edit = menuBar()->addMenu("&Edit");
    undo_act = edit->addAction("&Undo", Qt::CTRL + Qt::Key_Z, [this]() { mgr.undo(); log_msg("Undo"); });
    redo_act = edit->addAction("&Redo", Qt::CTRL + Qt::Key_Y, [this]() { mgr.redo(); log_msg("Redo"); });
    edit->addSeparator();
    sort_act = edit->addAction("&Sort Alphabetically", [this]() { mgr.sort_alpha(std::nullopt); log_msg("Sorted"); });
    sort_rec_act = edit->addAction("Sort &Recursively", [this]() { mgr.sort_alpha(std::nullopt); log_msg("Sorted Recursively"); });

    auto* help = menuBar()->addMenu("&Help");
    help->addAction("&About", [this]() { dsm::show_info(this, "About", "Disgaea DS Manager\nC++/Qt port"); });
}

void MainWindow::refresh_tree() {
    tree->clear();
    auto doc = mgr.current();
    if (!doc.has_content && doc.file_path.empty()) { root_item = nullptr; update_modified(); return; }
    auto* root = new QTreeWidgetItem(tree);
    root->setText(0, (doc.file_path.empty() ? "New" : fs::path(doc.file_path).filename().string())
                     + (doc.is_modified ? " *" : ""));
    root->setData(0, Qt::UserRole, QVariant::fromValue((qlonglong)doc.root->id));
    for (auto& c : doc.root->children) add_node(root, c);
    root_item = root;
    tree->addTopLevelItem(root);
    root->setExpanded(true);
    update_modified();
}

void MainWindow::add_node(QTreeWidgetItem* parent, const dsm::ArchiveEntryPtr& e) {
    auto* it = new QTreeWidgetItem(parent);
    it->setText(0, QString::fromStdString(e->display_name()));
    it->setData(0, Qt::UserRole, QVariant::fromValue((qlonglong)e->id));
    for (auto& c : e->children) add_node(it, c);
}

void MainWindow::on_open() {
    QString p = QFileDialog::getOpenFileName(this, "Open Archive", "", "Archives (*.dat *.msnd)");
    if (p.isEmpty()) return;
    try { mgr.load(p.toStdString()); log_msg("Opened " + p.toStdString()); }
    catch (std::exception& ex) { dsm::show_error(this, ex.what()); }
}

void MainWindow::on_save() {
    auto doc = mgr.current();
    if (doc.file_path.empty()) { on_save_as(); return; }
    if (mgr.is_root_empty()) { dsm::show_error(this, "Cannot save empty archive."); return; }
    if (mgr.has_blank_files()) {
        auto r = dsm::show_confirm_cancel(this, "Blank Files",
            "Archive has blank files. Remove them?\nYes=Remove, No=Keep, Cancel=Abort");
        if (r == dsm::ConfirmResult::Cancel) return;
        if (r == dsm::ConfirmResult::Yes) mgr.remove_all_empty();
    }
    mgr.save(doc.file_path);
    log_msg("Saved");
}

void MainWindow::on_save_as() {
    QString p = QFileDialog::getSaveFileName(this, "Save As", "", "Archives (*.dat *.msnd)");
    if (!p.isEmpty()) { mgr.save(p.toStdString()); log_msg("Saved As"); }
}

void MainWindow::on_new() {
    auto c = dsm::show_new_file_dialog(this);
    if (!c) return;
    dsm::ArchiveDocument d;
    d.file_type = *c == dsm::ArchiveTypeChoice::DSARC ? dsm::ArchiveType::DSARC : dsm::ArchiveType::MSND;
    d.has_content = true;
    if (d.file_type == dsm::ArchiveType::MSND) { d.root->nested_type = dsm::ArchiveType::MSND; d.root->data = dsm::build_empty_msnd(); }
    mgr.set_current(d);
    log_msg("Created new");
}

void MainWindow::on_context_menu(const QPoint& pos) {
    auto* item = tree->itemAt(pos);
    if (!item) return;
    int64_t id = item->data(0, Qt::UserRole).toLongLong();
    auto doc = mgr.current();
    auto e = dsm::find_by_id(doc.root, id);
    QMenu menu(this);
    if (!e) {
        menu.addAction("Import Folder", [this]() { on_import_folder(); });
        menu.addAction("Extract All", [this]() { dsm::extract_all(mgr.current(), pick_dir().toStdString(), false); });
    } else {
        menu.addAction("Extract", [this, e]() { dsm::extract_single(e, pick_dir().toStdString()); });
        menu.addAction("Rename", [this, e]() { on_rename(e); });
        menu.addAction("Delete", [this, e]() { if (dsm::show_confirm(this, "Delete", "Delete " + e->name + "?")) mgr.del(e); });
    }
    menu.exec(tree->viewport()->mapToGlobal(pos));
}

void MainWindow::on_rename(const dsm::ArchiveEntryPtr& e) {
    auto r = dsm::show_input(this, "Rename", "New name:", e->name);
    if (r && !r->empty() && *r != e->name) {
        if (mgr.has_duplicate(e, *r))
            if (!dsm::show_confirm(this, "Duplicate", "Name exists. Continue?")) return;
        mgr.rename(e, *r);
    }
}

void MainWindow::on_import_folder() {
    QString d = QFileDialog::getExistingDirectory(this, "Select Folder");
    if (!d.isEmpty()) { mgr.import_folder(d.toStdString()); log_msg("Imported"); }
}

QString MainWindow::pick_dir() {
    return QFileDialog::getExistingDirectory(this, "Output Folder");
}

void MainWindow::update_modified() {
    modified->setText(mgr.current().is_modified ? "● Modified" : "");
    update_edit_menu();
}

void MainWindow::update_edit_menu() {
    undo_act->setEnabled(mgr.can_undo());
    redo_act->setEnabled(mgr.can_redo());
    auto doc = mgr.current();
    sort_act->setEnabled(doc.file_type == dsm::ArchiveType::DSARC && doc.root->children.size() > 1);
    sort_rec_act->setEnabled(doc.file_type == dsm::ArchiveType::DSARC);
}

void MainWindow::log_msg(const std::string& m) {
    log->append(QString("[%1] %2").arg(QTime::currentTime().toString()).arg(QString::fromStdString(m)));
}
