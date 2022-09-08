//
// Created by TD on 2022/9/7.
//

#pragma once

#include <doodle_dingding/doodle_dingding_fwd.h>
#include <boost/asio/io_context.hpp>
namespace doodle::dingding {

class DOODLE_DINGDING_API client {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  explicit client(const boost::asio::io_context& in_context);
};

}  // namespace doodle::dingding
