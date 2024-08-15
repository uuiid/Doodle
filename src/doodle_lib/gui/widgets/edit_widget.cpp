
//
// Created by TD on 2021/9/23.
//

#include "edit_widget.h"

#include "doodle_core/metadata/assets_file.h"
#include "doodle_core/metadata/comment.h"
#include "doodle_core/metadata/episodes.h"
#include "doodle_core/metadata/importance.h"
#include "doodle_core/metadata/season.h"
#include "doodle_core/metadata/shot.h"
#include "doodle_core/metadata/time_point_wrap.h"
#include <doodle_core/metadata/file_one_path.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/metadata_cpp.h>

#include <doodle_lib/gui/widgets/assets_file_widgets.h>
#include <doodle_lib/gui/widgets/derail/assets_file_edit.h>
#include <doodle_lib/gui/widgets/derail/command_edit.h>
#include <doodle_lib/gui/widgets/derail/episodes_edit.h>
#include <doodle_lib/gui/widgets/derail/file_one_edit_t.h>
#include <doodle_lib/gui/widgets/derail/importance_edit.h>
#include <doodle_lib/gui/widgets/derail/season_edit.h>
#include <doodle_lib/gui/widgets/derail/shot_edit.h>
#include <doodle_lib/gui/widgets/derail/time_edit.h>
#include <doodle_lib/gui/widgets/derail/ue_main_map_edit.h>
#include <doodle_lib/gui/widgets/derail/user_edit.h>

#include <boost/hana.hpp>
#include <boost/hana/ext/std.hpp>
#include <boost/signals2/connection.hpp>

#include "entt/entity/fwd.hpp"
#include "fmt/core.h"
#include "range/v3/algorithm/for_each.hpp"
#include <memory>
#include <tuple>
namespace doodle::gui {

class edit_widgets::impl {
 public:
  template <typename T, typename Gui_T>
  class render_edit_impl {
   public:
    Gui_T edit{};
    impl *p_impl_{nullptr};
    explicit render_edit_impl(impl *in_impl) : p_impl_{in_impl} {}
    bool render(const entt::handle &in_handle) {
      bool on_change{false};
      if (edit.render(in_handle)) {
        auto l_data = in_handle.get<T>();
        ranges::for_each(p_impl_->p_h, [&](const entt::handle &in) {
          if (!in.any_of<T>()) in.emplace<T>();
          in.patch<T>() = l_data;
        });
        on_change = true;
      }
      return on_change;
    }
  };
  template <>
  class render_edit_impl<assets_file, render::assets_file_edit_t> {
   public:
    render::assets_file_edit_t edit{};
    impl *p_impl_{nullptr};
    explicit render_edit_impl(impl *in_impl) : p_impl_{in_impl} {}
    bool render(const entt::handle &in_handle) {
      bool on_change{false};
      if (edit.render(in_handle)) {
        auto l_data = in_handle.get<assets_file>().user_attr();
        ranges::for_each(p_impl_->p_h, [&](const entt::handle &in) {
          if (!in.any_of<assets_file>()) in.emplace<assets_file>();
          in.patch<assets_file>().user_attr(l_data);
        });
        on_change = true;
      }
      return on_change;
    }
  };

 public:
  /**
   * @brief 修改句柄
   *
   */
  std::vector<entt::handle> p_h;

  std::tuple<
      std::shared_ptr<render_edit_impl<season, render::season_edit_t>>,
      std::shared_ptr<render_edit_impl<episodes, render::episodes_edit_t>>,
      std::shared_ptr<render_edit_impl<shot, render::shot_edit_t>>,
      std::shared_ptr<render_edit_impl<assets_file, render::assets_file_edit_t>>,
      std::shared_ptr<render_edit_impl<comment, render::command_edit_t>>,
      std::shared_ptr<render_edit_impl<importance, render::importance_edit_t>>,
      std::shared_ptr<render_edit_impl<time_point_wrap, render::time_edit_t>>,
      std::shared_ptr<render_edit_impl<ue_main_map, render::ue_main_map_edit>>>
      render_data;

  std::string title_name_;
  bool open{true};
  boost::signals2::scoped_connection p_sc;
  std::string database_info{};
};

edit_widgets::edit_widgets() : p_i(std::make_unique<impl>()) {
  p_i->title_name_ = std::string{name};
  g_reg()->ctx().emplace<edit_widgets &>(*this);
  auto &l_sig      = g_ctx().get<core_sig>();
  p_i->p_sc        = l_sig.select_handles.connect([&](const std::vector<entt::handle> &in) {
    p_i->p_h = in;
    gen_text();
  });

  p_i->render_data = std::make_tuple(
      std::make_shared<impl::render_edit_impl<season, render::season_edit_t>>(p_i.get()),
      std::make_shared<impl::render_edit_impl<episodes, render::episodes_edit_t>>(p_i.get()),
      std::make_shared<impl::render_edit_impl<shot, render::shot_edit_t>>(p_i.get()),
      std::make_shared<impl::render_edit_impl<assets_file, render::assets_file_edit_t>>(p_i.get()),
      std::make_shared<impl::render_edit_impl<comment, render::command_edit_t>>(p_i.get()),
      std::make_shared<impl::render_edit_impl<importance, render::importance_edit_t>>(p_i.get()),
      std::make_shared<impl::render_edit_impl<time_point_wrap, render::time_edit_t>>(p_i.get()),
      std::make_shared<impl::render_edit_impl<ue_main_map, render::ue_main_map_edit>>(p_i.get())

  );
}
edit_widgets::~edit_widgets() = default;

bool edit_widgets::render() {
  this->edit_handle();
  return p_i->open;
}

void edit_widgets::edit_handle() {
  /// @brief 资产编辑
  if (!p_i->p_h.empty()) {
    const auto l_args = p_i->p_h.size();
    if (l_args > 1) dear::Text(fmt::format("同时编辑了 {}个", l_args));
    dear::Text(p_i->database_info);

    boost::hana::for_each(p_i->render_data, [&](auto &&i) { i->render(p_i->p_h.front()); });
  }
}

const std::string &edit_widgets::title() const { return p_i->title_name_; }
void edit_widgets::gen_text() {
  p_i->database_info.clear();
  for (auto i = 0; i < p_i->p_h.size() && i < 10; ++i) {
    p_i->database_info += fmt::format("id {} ", p_i->p_h[i].get<database>().get_id());
  }
  if (p_i->p_h.size() > 10) {
    p_i->database_info += "...";
  }
}

}  // namespace doodle::gui
