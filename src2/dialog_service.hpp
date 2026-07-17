#pragma once
#include <string>
#include <optional>

class QWidget;

namespace dsm {

enum class ConfirmResult { Yes, No, Cancel };

void show_info(QWidget* parent, const std::string& title, const std::string& msg);
void show_error(QWidget* parent, const std::string& msg);
void show_message(QWidget* parent, const std::string& title, const std::string& msg);
bool show_confirm(QWidget* parent, const std::string& title, const std::string& msg);
ConfirmResult show_confirm_cancel(QWidget* parent, const std::string& title, const std::string& msg);
std::optional<std::string> show_input(QWidget* parent, const std::string& title,
                                      const std::string& label, const std::string& def = "");
enum class ArchiveTypeChoice { DSARC, MSND };
std::optional<ArchiveTypeChoice> show_new_file_dialog(QWidget* parent);

} // namespace dsm
