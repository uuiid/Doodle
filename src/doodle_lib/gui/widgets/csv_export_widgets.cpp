//
// Created by TD on 2022/2/17.
//

#include "csv_export_widgets.h"
#include <core/core_sig.h>

namespace doodle {
namespace gui {

class csv_export_widgets::impl {
 public:
  std::vector<entt::handle> list;
  std::vector<boost::signals2::scoped_connection> con;
};

csv_export_widgets::csv_export_widgets()
    : p_i(std::make_unique<impl>()) {
}
csv_export_widgets::~csv_export_widgets() = default;

void csv_export_widgets::init() {
  g_reg()->set<csv_export_widgets &>(*this);
  if (auto l_l = g_reg()->try_ctx<std::vector<entt::handle>>(); l_l)
    p_i->list = *l_l;
  p_i->con.emplace_back(
      g_reg()->ctx<core_sig>().select_handles.connect(
          [this](const std::vector<entt::handle> &in) {
            p_i->list = in;
          }));
}
void csv_export_widgets::succeeded() {
  g_reg()->unset<csv_export_widgets>();
}
void csv_export_widgets::failed() {
  g_reg()->unset<csv_export_widgets>();
}
void csv_export_widgets::aborted() {
  g_reg()->unset<csv_export_widgets>();
}
void csv_export_widgets::update(const chrono::duration<
                                    chrono::system_clock::rep,
                                    chrono::system_clock::period> &,
                                void *data) {
}
}  // namespace gui
}  // namespace doodle
