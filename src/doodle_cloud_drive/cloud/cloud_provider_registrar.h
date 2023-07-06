//
// Created by td_main on 2023/7/5.
//

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

#include <windows.h>
#include <wil/result.h>
#include <sddl.h>
#include <unknwn.h>
#include <winternl.h>
#include <comutil.h>
#include <oleidl.h>
#include <ntstatus.h>
#include <cfapi.h>
// clang-format on
#include <doodle_core/doodle_core_fwd.h>

namespace doodle {

class cloud_fetch_data;

class cloud_provider_registrar {
 public:
  cloud_provider_registrar(FSys::path in_root, const FSys::path& in_server_path)
      : root_{std::move(in_root)}, server_root_{in_server_path} {
    if (!FSys::exists(root_)) FSys::create_directories(root_);
    init2();
    create_placeholder(in_server_path);
  }
  ~cloud_provider_registrar() { uninit2(); }

  [[nodiscard]] FSys::path& child_path() { return root_; }
  [[nodiscard]] const FSys::path& child_path() const { return root_; }

  [[nodiscard]] FSys::path& server_path() { return server_root_; }
  [[nodiscard]] const FSys::path& server_path() const { return server_root_; }

  std::map<std::int64_t, std::weak_ptr<cloud_fetch_data>> cloud_fetch_data_list{};

  inline static LARGE_INTEGER file_time_to_large_integer(_In_ const FILETIME in_filetime) {
    LARGE_INTEGER l_large_integer{};
    l_large_integer.LowPart  = in_filetime.dwLowDateTime;
    l_large_integer.HighPart = in_filetime.dwHighDateTime;
    return l_large_integer;
  };

  inline static LARGE_INTEGER longlong_to_large_integer(_In_ const std::size_t in_large_integer) {
    LARGE_INTEGER l_large_integer{};
    l_large_integer.QuadPart = in_large_integer;
    return l_large_integer;
  };

 private:
  CF_CONNECTION_KEY s_transferCallbackConnectionKey{};
  FSys::path root_{};
  FSys::path server_root_{};

  void list_dir_info(const FSys::path& in_parent);

  void create_placeholder(const FSys::path& in_parent);
  void init2();

  void uninit2();

  //  static std::wstring convert_sid_to_string_sid(::PSID p_sid) {
  //    std::wstring l_sid_string;
  //    if (!::ConvertSidToStringSidW(p_sid, l_sid_string.data())) {
  //      throw std::system_error{static_cast<int>(::GetLastError()), std::system_category()};
  //    }
  //    return l_sid_string;
  //  }
  //
  //  std::wstring get_sync_root_id() {
  //    auto l_token_user{get_token_information()};
  //    auto l_sid_string = convert_sid_to_string_sid(l_token_user->User.Sid);
  //    return fmt::format(L"Doodle.Drive.!{}!", l_sid_string);
  //  }
  //
  //  std::unique_ptr<::TOKEN_USER> get_token_information() {
  //    std::unique_ptr<::TOKEN_USER> l_token_info{};
  //    auto l_token_handle{::GetCurrentThreadEffectiveToken()};
  //    DWORD token_info_length{};
  //    if (!::GetTokenInformation(l_token_handle, ::TokenUser, nullptr, 0, &token_info_length)) {
  //      if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
  //        throw std::system_error{static_cast<int>(::GetLastError()), std::system_category()};
  //      }
  //
  //      l_token_info.reset(reinterpret_cast<::TOKEN_USER*>(new char[token_info_length]));
  //      if (!::GetTokenInformation(
  //              l_token_handle, ::TokenUser, l_token_info.get(), token_info_length, &token_info_length
  //          )) {
  //        throw std::system_error{static_cast<int>(::GetLastError()), std::system_category()};
  //      }
  //    }
  //    return l_token_info;
  //  }
};
}  // namespace doodle
