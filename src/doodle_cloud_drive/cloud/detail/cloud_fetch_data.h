//
// Created by td_main on 2023/7/8.
//

#pragma once
#include <doodle_core/core/file_sys.h>
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
#include <boost/asio.hpp>

#include <memory>
namespace doodle::detail {

class cloud_fetch_data : public std::enable_shared_from_this<cloud_fetch_data> {
  constexpr static std::size_t buffer_size = 4096;

 public:
  explicit cloud_fetch_data(
      boost::asio::io_context& io_context, FSys::path in_server_path, FSys::path in_child_path,
      CF_CALLBACK_INFO in_callback_info_, const CF_CALLBACK_PARAMETERS* in_callback_parameters
  )
      : stream_handle_{io_context},
        server_path_{std::move(in_server_path)},
        child_path_{std::move(in_child_path)},
        length_{std::min(
            boost::numeric_cast<std::size_t>(in_callback_parameters->FetchData.RequiredLength.QuadPart), buffer_size
        )},
        buffer_{std::make_unique<char[]>(length_)},
        callback_info_{in_callback_info_},
        start_offset_{boost::numeric_cast<std::size_t>(in_callback_parameters->FetchData.RequiredFileOffset.QuadPart)},
        remaining_length_{in_callback_parameters->FetchData.RequiredLength} {
    init();
  }

 private:
  boost::asio::windows::random_access_handle stream_handle_;
  FSys::path server_path_;
  FSys::path child_path_;
  std::size_t length_;
  std::unique_ptr<char[]> buffer_;
  CF_CALLBACK_INFO callback_info_;
  std::size_t start_offset_{};
  LARGE_INTEGER remaining_length_{};

  void init();

 public:
  void async_read();

  void cancel() { stream_handle_.cancel(); }

  void transfer_data(
      _In_ CF_CONNECTION_KEY connectionKey, _In_ LARGE_INTEGER transferKey,
      _In_reads_bytes_opt_(length.QuadPart) LPCVOID transferData, _In_ std::size_t startingOffset,
      _In_ std::size_t length, _In_ NTSTATUS completionStatus
  );
};

}  // namespace doodle::detail
