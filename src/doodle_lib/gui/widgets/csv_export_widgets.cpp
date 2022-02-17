//
// Created by TD on 2022/2/17.
//

#include "csv_export_widgets.h"

namespace doodle {
namespace gui {

class csv_export_widgets::impl {
 public:
};

csv_export_widgets::csv_export_widgets()
    : p_i(std::make_unique<impl>()) {
}
csv_export_widgets::~csv_export_widgets() = default;

void csv_export_widgets::init() {
}
void csv_export_widgets::succeeded() {
}
void csv_export_widgets::failed() {
}
void csv_export_widgets::aborted() {
}
void csv_export_widgets::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void *data) {
}
}  // namespace gui
}  // namespace doodle
