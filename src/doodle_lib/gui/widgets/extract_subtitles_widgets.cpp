//
// Created by TD on 2022/4/18.
//

#include "extract_subtitles_widgets.h"

namespace doodle::gui {
class extract_subtitles_widgets::impl {
 public:
};

extract_subtitles_widgets::extract_subtitles_widgets()
    : p_i(std::make_unique<impl>()) {
}
void extract_subtitles_widgets::render() {
}
extract_subtitles_widgets::~extract_subtitles_widgets() = default;
}  // namespace doodle::gui
