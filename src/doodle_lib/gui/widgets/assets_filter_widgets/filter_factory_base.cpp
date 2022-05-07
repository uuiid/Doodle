//
// Created by TD on 2022/5/7.
//

#include "filter_factory_base.h"

namespace doodle::gui {
class filter_factory_base::impl {
 public:
  std::vector<boost::signals2::scoped_connection> p_conns;
  bool need_init{false};
};

filter_factory_base::filter_factory_base()
    : p_i(std::make_unique<impl>()),
      is_disabled(false),
      p_obs(),
      is_edit(false) {
  connection_sig();
}

std::unique_ptr<filter_base> filter_factory_base::make_filter() {
  is_edit = false;
  return make_filter_();
}
void filter_factory_base::refresh(bool force) {
  if (!is_disabled)
    refresh_();

  p_obs.clear();

  if (p_i->need_init || force) {
    this->init();
    p_i->need_init = false;
  }
}
void filter_factory_base::connection_sig() {
  auto& l_sig = g_reg()->ctx().at<core_sig>();

  p_i->p_conns.emplace_back(l_sig.project_begin_open.connect(
      [&](const std::filesystem::path&) {
        this->is_disabled = true;
      }));
  p_i->p_conns.emplace_back(l_sig.project_end_open.connect(
      [&](const entt::handle&, const doodle::project&) {
        this->is_disabled = false;
        p_i->need_init    = true;
      }));
  p_i->p_conns.emplace_back(l_sig.save_end.connect(
      [&](const std::vector<entt::handle>&) {
        p_i->need_init = true;
      }));
}
filter_factory_base::~filter_factory_base() = default;

}  // namespace doodle::gui
