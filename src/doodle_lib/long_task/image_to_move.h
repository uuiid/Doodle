//
// Created by TD on 2021/12/27.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_core/thread_pool/image_to_movie.h>
#include <doodle_core/metadata/move_create.h>
#include <opencv2/core/types.hpp>

#include <boost/asio.hpp>

namespace doodle {

namespace detail {

class DOODLELIB_API image_to_move : public image_to_movie_interface {
 public:
  using image_attr      = ::doodle::movie::image_attr;
  using image_watermark = ::doodle::movie::image_watermark;

 private:
  class impl;
  std::unique_ptr<impl> p_i;

  FSys::path create_out_path(const entt::handle &in_handle) override;

 public:
  image_to_move();
  virtual ~image_to_move();

  virtual void create_move(
      const FSys::path &in_out_path,
      process_message &in_msg,
      const std::vector<image_attr> &in_vector
  ) override;
};
}  // namespace detail

}  // namespace doodle
