//
// Created by TD on 2024/2/29.
//

#include "work.h"

#include "doodle_core/exception/exception.h"
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/http/websocket_route.h>
#include <doodle_lib/exe_warp/windows_hide.h>
#include <doodle_lib/lib_warp/boost_fmt_error.h>

#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/process/process.hpp>

#include <core/http/json_body.h>
#include <spdlog/sinks/basic_file_sink.h>

// 使用 win32 GetSystemFirmwareTable api 获取主板(RSMB)的uuid

#include <spdlog/spdlog.h>
#include <windows.h>

namespace doodle::http {

namespace {
uuid get_motherboard_uuid() {
  constexpr UINT kFirmwareTableId = 'RSMB';

  // 获取固件表的大小
  UINT size                       = GetSystemFirmwareTable(kFirmwareTableId, 0, nullptr, 0);
  DOODLE_CHICK(size > 0, "无法获取固件表大小");

  std::vector<BYTE> buffer(size);
  UINT read_size = GetSystemFirmwareTable(kFirmwareTableId, 0, buffer.data(), size);
  DOODLE_CHICK(read_size == size, "无法读取固件表数据");

#pragma pack(push, 1)
  struct RawSMBIOSData {
    BYTE Used20CallingMethod;
    BYTE SMBIOSMajorVersion;
    BYTE SMBIOSMinorVersion;
    BYTE DmiRevision;
    DWORD Length;
    BYTE SMBIOSTableData[];
  };
  struct SMBIOSHeader {
    BYTE type;
    BYTE length;
    WORD handle;
  };
  struct SMBIOS_SYSTEM_INFO {
    SMBIOSHeader Header;

    uint8_t Manufacturer;
    uint8_t ProductName;
    uint8_t Version;
    uint8_t SerialNumber;

    uint8_t UUID[16];

    uint8_t WakeUpType;
  };
#pragma pack(pop)
  // SMBIOS 数据结构：前 8 字节为 header，之后是 SMBIOS tables
  // UUID 通常在 Type 1 (System Information) 结构中，偏移为 8 字节
  DOODLE_CHICK(size >= sizeof(RawSMBIOSData), "固件表数据过小，无法包含 SMBIOS 头");
  auto raw_data                = reinterpret_cast<RawSMBIOSData*>(buffer.data());
  const BYTE* smbios_table     = raw_data->SMBIOSTableData;
  const UINT smbios_table_size = raw_data->Length;

  for (UINT i = 0; i + sizeof(SMBIOSHeader) <= smbios_table_size;) {
    auto header = reinterpret_cast<const SMBIOSHeader*>(smbios_table + i);
    DOODLE_CHICK(header->length >= sizeof(SMBIOSHeader), "SMBIOS 结构头长度无效");
    DOODLE_CHICK(i + header->length <= smbios_table_size, "SMBIOS 结构越界");

    if (header->type == 1 && header->length >= sizeof(SMBIOS_SYSTEM_INFO)) {  // Type 1: System Information
      const auto* system_info = reinterpret_cast<const SMBIOS_SYSTEM_INFO*>(smbios_table + i);
      const auto& uuid_ptr    = system_info->UUID;
      boost::uuids::uuid l_uuid{{
          uuid_ptr[3], uuid_ptr[2], uuid_ptr[1], uuid_ptr[0],  // Data1 (4 bytes, little-endian)
          uuid_ptr[5], uuid_ptr[4],                            // Data2 (2 bytes, little-endian)
          uuid_ptr[7], uuid_ptr[6],                            // Data3 (2 bytes, little-endian)
          uuid_ptr[8], uuid_ptr[9], uuid_ptr[10], uuid_ptr[11], uuid_ptr[12], uuid_ptr[13], uuid_ptr[14],
          uuid_ptr[15]  // Data4 (8 bytes, big-endian)
      }};
      SPDLOG_WARN("获取到主板uuid {}", l_uuid);
      return l_uuid;
    }

    UINT next                  = i + header->length;
    bool has_string_terminator = false;
    while (next + 1 < smbios_table_size) {
      if (smbios_table[next] == 0 && smbios_table[next + 1] == 0) {
        next += 2;
        has_string_terminator = true;
        break;
      }
      ++next;
    }

    DOODLE_CHICK(has_string_terminator, "SMBIOS 字符串区未正确结束");
    i = next;
  }

  throw_exception(doodle_error{"无法找到主板 UUID"});
}

}  // namespace

boost::asio::awaitable<tl::expected<http_work::run_task_info, std::string>> http_work::get_task_data() {
  run_task_info l_info{};

  co_return tl::expected<http_work::run_task_info, std::string>{l_info};
}

void http_work::run(const boost::urls::url& in_url) {
  executor_ = boost::asio::make_strand(g_io_context());
  timer_    = std::make_shared<timer>(executor_);
  url_      = in_url;

  logger_   = g_logger_ctrl().make_log("http_work");
  boost::asio::co_spawn(
      executor_, async_run(),
      boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
  );
}

boost::asio::awaitable<void> http_work::async_run() {
  computer l_computer_data{.hardware_id_ = get_motherboard_uuid(), .name_ = boost::asio::ip::host_name()};
  auto l_websocket_client = co_await make_websocket_stream(url_);
  co_await l_websocket_client->async_write(boost::asio::buffer(nlohmann::json(l_computer_data).dump()));

  co_return;
}
boost::asio::awaitable<void> http_work::async_run_task() { co_return; }

boost::asio::awaitable<void> http_work::async_read_pip(std::shared_ptr<boost::asio::readable_pipe> in_pipe) {
  co_return;
}

boost::asio::awaitable<void> http_work::async_set_status(computer_status in_status) { co_return; }
boost::asio::awaitable<void> http_work::async_set_task_status(server_task_info_status in_status) { co_return; }

}  // namespace doodle::http