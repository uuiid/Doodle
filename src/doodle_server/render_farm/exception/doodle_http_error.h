//
// Created by td_main on 2023/8/9.
//

#pragma once

#include <boost/beast/http/message_generator.hpp>

#include <filesystem>
#include <stdexcept>
#include <string>

namespace doodle {

class doodle_http_error : public std::runtime_error {
  boost::beast::http::message_generator message_generator_;

 public:
  template <bool isRequest, class Body, class Fields>
  explicit doodle_http_error(std::string in_str, boost::beast::http::message<isRequest, Body, Fields>&& in_message)
      : std::runtime_error(in_str), message_generator_(std::move(in_message)) {}

  boost::beast::http::message_generator& message_generator() { return message_generator_; }
};

}  // namespace doodle