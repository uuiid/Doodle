//
// Created by TD on 2021/9/15.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/imgui_warp.h>

#include <boost/hana/experimental/printable.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/free.hpp>
#include <boost/type_erasure/member.hpp>

namespace doodle {
template <class Panel>
class DOODLELIB_API base_window : public process_t<base_window<Panel>> {
 public:
  using self_type    = base_window<Panel>;
  using base_type    = process_t<base_window<Panel>>;
  using derived_type = Panel;

  template <class... Args>
  explicit base_window(Args&&... args)
      : panel_(std::forward<Args>(args)...) {}

  Panel panel_;
  bool show{true};
  [[maybe_unused]] virtual void init() {
    g_reg()->template set<self_type&>(*this);
    panel_.init();
  }
  [[maybe_unused]] virtual void succeeded() {
    panel_.succeeded();
  }
  [[maybe_unused]] virtual void failed() {
    panel_.failed();
  }
  [[maybe_unused]] virtual void aborted() {
    panel_.aborted();
  }
  [[maybe_unused]] virtual void update(typename base_type::delta_type in_dalta, void* in_data) {
    if (!show)
      this->succeed();

    dear::Begin{panel_.name.data(), &show} &&
        [&]() {
          panel_.update(in_dalta, in_data);
        };
  }
};
template <class Panel, class... Args>
auto make_windows(Args&&... args) {
  if (auto k_panel = g_reg()->try_ctx<Panel>(); !k_panel)
    g_main_loop().attach<base_window<Panel>>(std::forward<Args>(args)...);
}

BOOST_TYPE_ERASURE_MEMBER(render);

using widget_ = boost::type_erasure::any<
    boost::mpl::vector<
        has_render<bool()>,
        boost::type_erasure::copy_constructible<>,
        boost::type_erasure::typeid_<>,
        boost::type_erasure::relaxed>>;

}  // namespace doodle
