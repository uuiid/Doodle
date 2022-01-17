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

template <class Derived>
class DOODLELIB_API base_window : public process_t<base_window<Derived>> {
 public:
  using base_type    = process_t<base_window<Derived>>;
  using derived_type = Derived;

  bool show;
  [[maybe_unused]] virtual void init() {
    g_reg()->template set<derived_type&>(*(static_cast<derived_type*>(this)));
    static_cast<derived_type*>(this)->init();
  }
  [[maybe_unused]] virtual void succeeded() {
    static_cast<derived_type*>(this)->succeeded();
  }
  [[maybe_unused]] virtual void failed() {
    static_cast<derived_type*>(this)->failed();
  }
  [[maybe_unused]] virtual void aborted() {
    static_cast<derived_type*>(this)->aborted();
  }
  [[maybe_unused]] virtual void update(typename base_type::delta_type in_dalta, void* in_data) {
    dear::Begin{derived_type::name.data(), &show} && [&]() {
      static_cast<derived_type*>(this)->update(in_dalta, in_data);
    };
  }
};

BOOST_TYPE_ERASURE_MEMBER(render);

using widget_ = boost::type_erasure::any<
    boost::mpl::vector<
        has_render<bool()>,
        boost::type_erasure::copy_constructible<>,
        boost::type_erasure::typeid_<>,
        boost::type_erasure::relaxed>>;

}  // namespace doodle
