//
// Created by TD on 2021/10/14.
//

#pragma once
#include <doodle_core/core/app_facet.h>
#include <doodle_app/app/facet/gui_facet.h>
#include <doodle_core/thread_pool/image_to_movie.h>
namespace doodle::maya_plug {

namespace detail {

class maya_create_movie : public doodle::detail::image_to_movie_interface {
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  maya_create_movie();
  virtual maya_create_movie();

  void create_move(
      const FSys::path& in_out_path,
      process_message& in_msg,
      const std::vector<image_attr>& in_vector
  ) override;

 protected:
  FSys::path create_out_path(const entt::handle& in_handle) override;
};

}  // namespace detail

class maya_facet : public doodle::facet::gui_facet {
 protected:
  void load_windows() override;

 public:
  void close_windows() override;
};

}  // namespace doodle::maya_plug
