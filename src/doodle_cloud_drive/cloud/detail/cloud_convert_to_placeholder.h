//
// Created by td_main on 2023/7/10.
//

#pragma once
#include <doodle_core/core/file_sys.h>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/io_context.hpp>

#include <memory>
// clang-format off
#include <windows.h>
#include <ntstatus.h>
// clang-format on
namespace doodle::detail {

class cloud_convert_to_placeholder : public std::enable_shared_from_this<cloud_convert_to_placeholder> {
 public:
  explicit cloud_convert_to_placeholder(
      boost::asio::io_context& in_context, FSys::path in_server_path, FSys::path in_child_path
  )
      : executor_(in_context.get_executor()),
        server_path_{std::move(in_server_path)},
        child_path_{std::move(in_child_path)} {}
  ~cloud_convert_to_placeholder() = default;

  void async_run();

 private:
  boost::asio::any_io_executor executor_;
  FSys::path server_path_;
  FSys::path child_path_;
  NTSTATUS ntstatus_{STATUS_SUCCESS};
  void async_convert_to_placeholder();
};

}  // namespace doodle::detail
