//
// Created by td_main on 2023/7/10.
//

#pragma once
#include "doodle_core/core/file_sys.h"

#include "boost/asio/any_io_executor.hpp"
#include "boost/asio/io_context.hpp"

#include <cstdint>
#include <memory>
// clang-format off
#include <Windows.h>
#include "wil/result.h"
#include <sddl.h>
#include <Unknwn.h>
#include <winternl.h>
#include <comutil.h>
#include <oleidl.h>
#include <ntstatus.h>
#include <cfapi.h>
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
  template <typename Executor_Type>
  explicit cloud_convert_to_placeholder(Executor_Type in_context, FSys::path in_server_path, FSys::path in_child_path)
      : executor_(std::move(in_context)),
        server_path_{std::move(in_server_path)},
        child_path_{std::move(in_child_path)} {}
  ~cloud_convert_to_placeholder() = default;

  void async_run();

 private:
  boost::asio::any_io_executor executor_;
  FSys::path server_path_;
  FSys::path child_path_;
  NTSTATUS ntstatus_{STATUS_SUCCESS};
  bool sub_run{};
  void async_convert_to_placeholder();
  void init_child_placeholder();

  struct sub_path {
    std::int32_t deep_{};
    FSys::path path_{};
  };
};

}  // namespace doodle::detail
