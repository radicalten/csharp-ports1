#include "dialog_service.hpp"
#include <QMessageBox>
#include <QInputDialog>

namespace dsm {

void show_info(QWidget* p, const std::string& t, const std::string& m) {
    QMessageBox::about(p, QString::fromStdString(t), QString::fromStdString(m));
}
void show_error(QWidget* p, const std::string& m) {
    QMessageBox::critical(p, "Error", QString::fromStdString(m));
}
void show_message(QWidget* p, const std::string& t, const std::string& m) {
    QMessageBox::information(p, QString::fromStdString(t), QString::fromStdString(m));
}
bool show_confirm(QWidget* p, const std::string& t, const std::string& m) {
    return QMessageBox::question(p, QString::fromStdString(t), QString::fromStdString(m),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}
ConfirmResult show_confirm_cancel(QWidget* p, const std::string& t, const std::string& m) {
    auto r = QMessageBox::question(p, QString::fromStdString(t), QString::fromStdString(m),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    return r == QMessageBox::Yes ? ConfirmResult::Yes :
           r == QMessageBox::No ? ConfirmResult::No : ConfirmResult::Cancel;
}
std::optional<std::string> show_input(QWidget* p, const std::string& t, const std::string& l, const std::string& d) {
    bool ok;
    QString r = QInputDialog::getText(p, QString::fromStdString(t), QString::fromStdString(l),
        QLineEdit::Normal, QString::fromStdString(d), &ok);
    return ok ? std::optional<std::string>(r.toStdString()) : std::nullopt;
}
std::optional<ArchiveTypeChoice> show_new_file_dialog(QWidget* p) {
    QStringList items{"DSARC (.dat)", "DSEQ (.msnd)"};
    bool ok;
    QString r = QInputDialog::getItem(p, "New Archive", "Select type:", items, 0, false, &ok);
    if (!ok) return std::nullopt;
    return r.startsWith("DSARC") ? ArchiveTypeChoice::DSARC : ArchiveTypeChoice::MSND;
}

} // namespace dsm
