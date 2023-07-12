//
// Created by td_main on 2023/7/12.
//

#include "upload_files.h"

namespace doodle::gui {
const std::string& upload_files::title() const { return title_; }
bool upload_files::render() { return show_; }
}  // namespace doodle::gui