//
// Created by TD on 2022/5/27.
//

#include "window.h"

#include <doodle_lib/gui/widgets/edit_widget.h>
#include <doodle_lib/gui/widgets/assets_filter_widget.h>
#include <doodle_lib/gui/widgets/csv_export_widgets.h>
#include <doodle_lib/gui/widgets/ue4_widget.h>
#include <doodle_lib/gui/widgets/extract_subtitles_widgets.h>
#include <doodle_lib/gui/widgets/subtitle_processing.h>
#include <doodle_lib/gui/widgets/assets_file_widgets.h>
#include <doodle_lib/gui/widgets/long_time_tasks_widget.h>
#include <doodle_lib/gui/widgets/time_sequencer_widget.h>
void limited_layout::init() {
  g_reg()->ctx().emplace<layout_window &>(*this);
  boost::asio::post(g_io_context(), [this]() {
    call_render<::doodle::edit_widgets>();
    call_render<::doodle::assets_filter_widget>();
    call_render<::doodle::comm_maya_tool>();
    ;
    call_render<::doodle::assets_file_widgets>();
    call_render<::doodle::long_time_tasks_widget>();
  });
}
