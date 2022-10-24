//
// Created by TD on 2022/10/13.
//

#pragma once
#include <doodle_core/thread_pool/image_to_movie.h>

#include <maya_plug/main/maya_plug_fwd.h>
namespace doodle::maya_plug::detail {

class maya_create_movie : public doodle::detail::image_to_movie_interface {
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  maya_create_movie();
  virtual ~maya_create_movie();

  void create_move(
      const FSys::path& in_out_path,
      process_message& in_msg,
      const std::vector<image_attr>& in_vector
  ) override;

 protected:
  FSys::path create_out_path(const entt::handle& in_handle) override;
};

}  // namespace doodle::maya_plug::detail
