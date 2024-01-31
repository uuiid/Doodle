//
// Created by td_main on 2023/7/8.
//

#pragma once
#include "doodle_core/core/file_sys.h"
// clang-format off
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cwchar>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

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
#include "boost/asio.hpp"

#include <memory>
namespace doodle::detail {
class cloud_fetch_placeholders : public std::enable_shared_from_this<cloud_fetch_placeholders> {
 public:
  explicit cloud_fetch_placeholders(boost::asio::io_context&, FSys::path in_server_path, FSys::path in_child_path, CF_CALLBACK_INFO in_callback_info_, const CF_CALLBACK_PARAMETERS*)
      : server_path_{std::move(in_server_path)},
        child_path_{std::move(in_child_path)},
        search_path_{reinterpret_cast<wchar_t const*>(in_callback_info_.FileIdentity)},
        create_placeholder_path_{FSys::path{in_callback_info_.VolumeDosName} / in_callback_info_.NormalizedPath},
        callback_info_{in_callback_info_} {}
  ~cloud_fetch_placeholders() = default;
  void async_run();

 private:
  struct data_value {
    std::wstring relative_file_name;
    std::wstring file_identity;
  };
  FSys::path server_path_;
  FSys::path child_path_;
  /// 服务器搜索路径
  FSys::path search_path_;
  /// 创建占位符的路径
  FSys::path create_placeholder_path_;
  /// 回调信息
  CF_CALLBACK_INFO callback_info_;
  std::vector<CF_PLACEHOLDER_CREATE_INFO> placeholder_create_infos_;
  std::vector<std::shared_ptr<data_value>> data_values_;

  std::size_t file_count_{};

  NTSTATUS ntstatus_{STATUS_SUCCESS};

  CF_OPERATION_TRANSFER_PLACEHOLDERS_FLAGS flags_{CF_OPERATION_TRANSFER_PLACEHOLDERS_FLAG_NONE};
  CF_PLACEHOLDER_CREATE_INFO placeholder_create_info_{};
  void init();

  void transfer_data();
  bool transfer_data_one();
  // 失败
  void fail();
};

}  // namespace doodle::detail
