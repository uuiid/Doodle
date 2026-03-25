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
#include <doodle_lib/exe_warp/export_fbx_arg.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/windows_hide.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/lib_warp/boost_fmt_error.h>

#include <boost/asio/consign.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/lockfree/detail/uses_optional.hpp>
#include <boost/process/process.hpp>
#include <boost/scope/scope_exit.hpp>
#include <boost/url/url.hpp>

#include "core/global_function.h"
#include <core/http/json_body.h>
#include <memory>
#include <spdlog/sinks/basic_file_sink.h>

// 使用 win32 GetSystemFirmwareTable api 获取主板(RSMB)的uuid

#include <spdlog/spdlog.h>
#include <windows.h>

namespace doodle::http {

namespace {
bool uses_smbios_rfc4122_byte_order(BYTE major_version, BYTE minor_version) {
  return major_version > 2 || (major_version == 2 && minor_version >= 6);
}

boost::uuids::uuid convert_smbios_uuid(const uint8_t (&uuid_ptr)[16], BYTE major_version, BYTE minor_version) {
  if (uses_smbios_rfc4122_byte_order(major_version, minor_version)) {
    return boost::uuids::uuid{
        {uuid_ptr[3], uuid_ptr[2], uuid_ptr[1], uuid_ptr[0], uuid_ptr[5], uuid_ptr[4], uuid_ptr[7], uuid_ptr[6],
         uuid_ptr[8], uuid_ptr[9], uuid_ptr[10], uuid_ptr[11], uuid_ptr[12], uuid_ptr[13], uuid_ptr[14], uuid_ptr[15]}
    };
  }

  return boost::uuids::uuid{
      {uuid_ptr[0], uuid_ptr[1], uuid_ptr[2], uuid_ptr[3], uuid_ptr[4], uuid_ptr[5], uuid_ptr[6], uuid_ptr[7],
       uuid_ptr[8], uuid_ptr[9], uuid_ptr[10], uuid_ptr[11], uuid_ptr[12], uuid_ptr[13], uuid_ptr[14], uuid_ptr[15]}
  };
}

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
      auto l_uuid = convert_smbios_uuid(uuid_ptr, raw_data->SMBIOSMajorVersion, raw_data->SMBIOSMinorVersion);
      SPDLOG_WARN(
          "获取到主板uuid {}, SMBIOS 版本 {}.{}", l_uuid, static_cast<int>(raw_data->SMBIOSMajorVersion),
          static_cast<int>(raw_data->SMBIOSMinorVersion)
      );
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

void http_work::run(const std::string& in_token) {
  executor_ = boost::asio::make_strand(g_io_context());
  token_    = in_token;
  spdlog::flush_every(1s);
  logger_ = g_logger_ctrl().make_log("http_work");
  boost::asio::co_spawn(
      executor_, async_run(),
      boost::asio::bind_cancellation_slot(
          app_base::Get().on_cancel.slot(), boost::asio::consign(boost::asio::detached, shared_from_this())
      )
  );
}

boost::asio::awaitable<void> http_work::async_run() {
  this_computer_info_.hardware_id_ = get_motherboard_uuid();
  this_computer_info_.name_        = boost::asio::ip::host_name();
  this_computer_info_.status_      = computer_status::online;
  auto l_ip                        = core_set::get_set().server_ip;
  if (l_ip.starts_with("http://"))
    l_ip.erase(0, 7);
  else if (l_ip.starts_with("https://"))
    l_ip.erase(0, 8);
  const boost::urls::url l_url{fmt::format("ws://{}/api/data/computers", l_ip)};
  for (auto i = 0;
       (co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none &&
       i < 100;
       ++i) {
    try {
      websocket_client_          = co_await make_websocket_stream(l_url);
      const auto l_computer_json = nlohmann::json(this_computer_info_).dump();
      co_await websocket_client_->async_write(boost::asio::buffer(l_computer_json));
      set_computer_status(computer_status::online);
      begin_ping();
      while ((co_await boost::asio::this_coro::cancellation_state).cancelled() ==
             boost::asio::cancellation_type::none) {
        boost::beast::flat_buffer l_msg;
        co_await websocket_client_->async_read(l_msg);
        // 处理消息
        auto l_json =
            nlohmann::json::parse(boost::asio::buffers_begin(l_msg.data()), boost::asio::buffers_end(l_msg.data()));
        auto l_data     = l_json.get<server_task_info>();
        l_data.uuid_id_ = l_json.at("id").get<uuid>();
        l_data.command_ = l_json.at("command");

        SPDLOG_LOGGER_INFO(logger_, "收到任务 {}，命令 {}", l_data.uuid_id_, l_data.command_.dump());
        if (run_task(l_data))
          set_computer_status(computer_status::busy);
        else {
          SPDLOG_LOGGER_ERROR(logger_, "无法运行任务 {}，不支持的任务类型 {}", l_data.uuid_id_, l_data.type_);
          set_computer_status(computer_status::online);
        }
      }
    } catch (const boost::system::system_error& e) {
      SPDLOG_LOGGER_ERROR(logger_, "WebSocket 连接发生错误: {}", e.what());
    } catch (const std::exception& e) {
      SPDLOG_LOGGER_ERROR(logger_, "处理 WebSocket 消息发生错误: {}", e.what());
    }
  }

  co_return;
}
bool http_work::run_task(const server_task_info& in_task_info) {
  if (in_task_info.type_ == server_task_info_type::auto_light) {
    auto l_run = std::make_shared<run_ue_assembly_distributed>(in_task_info, token_, shared_from_this());
    boost::asio::co_spawn(executor_, l_run->run(), boost::asio::consign(boost::asio::detached, l_run));
    return true;
  }
  if (in_task_info.type_ == server_task_info_type::export_fbx) {
    auto l_run = std::make_shared<export_fbx_arg_distributed>(in_task_info, token_, shared_from_this());
    boost::asio::co_spawn(executor_, l_run->run(), boost::asio::consign(boost::asio::detached, l_run));
    return true;
  }
  return false;
}

void http_work::set_computer_status(computer_status in_status) {
  this_computer_info_.status_ = in_status;
  message_queue_.push(nlohmann::json(this_computer_info_).dump());
  begin_write_msg();
}

void http_work::begin_write_msg() {
  if (is_writing_) return;
  boost::asio::co_spawn(
      strand_, async_write_msg(),
      boost::asio::bind_cancellation_slot(
          app_base::Get().on_cancel.slot(), boost::asio::consign(boost::asio::detached, shared_from_this())
      )
  );
}

void http_work::begin_ping() {
  boost::asio::co_spawn(
      g_io_context(), async_ping_loop(),
      boost::asio::bind_cancellation_slot(
          app_base::Get().on_cancel.slot(), boost::asio::consign(boost::asio::detached, shared_from_this())
      )
  );
}

boost::asio::awaitable<void> http_work::async_write_msg() {
  DOODLE_TO_EXECUTOR(strand_);
  if (is_writing_) co_return;
  is_writing_ = true;
  boost::scope::scope_exit l_{[this]() { is_writing_ = false; }};

  try {
    if (ping_message_.read(boost::lockfree::uses_optional)) {
      co_await websocket_client_->async_ping(boost::beast::websocket::ping_data{}, boost::asio::use_awaitable);
    }

    while (!message_queue_.empty()) {
      std::string l_msg{};
      message_queue_.pop(l_msg);
      if (!websocket_client_) break;
      if (!l_msg.empty())
        co_await websocket_client_->async_write(boost::asio::buffer(l_msg), boost::asio::use_awaitable);
      else
        co_await websocket_client_->async_ping(boost::beast::websocket::ping_data{}, boost::asio::use_awaitable);
    }
  } catch (const boost::system::system_error& e) {
    SPDLOG_LOGGER_ERROR(logger_, "WebSocket 连接发生错误: {}", e.what());
  } catch (const std::exception& e) {
    SPDLOG_LOGGER_ERROR(logger_, "处理 WebSocket 消息发生错误: {}", e.what());
  }
}
template <class Mutex>
class run_ue_assembly_distributed_sink : public spdlog::sinks::base_sink<Mutex>,
                                         public std::enable_shared_from_this<run_ue_assembly_distributed_sink<Mutex>> {
  std::once_flag flag_;
  std::shared_ptr<kitsu::kitsu_client> kitsu_client_;
  uuid job_id_;
  std::array<std::string, 2> log_buffer_;
  std::atomic_int log_index_{0};
  boost::asio::strand<boost::asio::io_context::executor_type> strand_{boost::asio::make_strand(g_io_context())};

 public:
  explicit run_ue_assembly_distributed_sink(std::shared_ptr<kitsu::kitsu_client> in_kitsu_client, uuid in_job_id)
      : kitsu_client_(in_kitsu_client), job_id_(in_job_id) {
    log_buffer_[log_index_].reserve(1024 * 10);
  }
  void sink_it_(const spdlog::details::log_msg& msg) override {
    spdlog::memory_buf_t formatted{};
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    log_buffer_[log_index_] += fmt::to_string(formatted);
  }
  void flush_() override {
    if (log_buffer_[log_index_].empty()) return;
    log_index_ = !log_index_;
    kitsu_client_->put_job_log_sync(job_id_, log_buffer_[!log_index_]);
    log_buffer_[!log_index_].clear();
  }
};
using run_ue_assembly_distributed_sink_mt = run_ue_assembly_distributed_sink<std::mutex>;

boost::asio::awaitable<void> http_work::async_ping_loop() {
  try {
    boost::asio::steady_timer timer{co_await boost::asio::this_coro::executor};
    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none &&
           websocket_client_ && websocket_client_->is_open()) {
      timer.expires_after(std::chrono::seconds(30));
      co_await timer.async_wait(boost::asio::use_awaitable);
      ping_message_.write(boost::beast::websocket::ping_data{});
      begin_write_msg();
    }
  } catch (const boost::system::system_error& e) {
    SPDLOG_LOGGER_ERROR(logger_, "WebSocket 连接发生错误: {}", e.what());
  } catch (const std::exception& e) {
    SPDLOG_LOGGER_ERROR(logger_, "处理 WebSocket 消息发生错误: {}", e.what());
  }
}
base_distributed_task::~base_distributed_task() {
  if (http_work_ptr_) http_work_ptr_->set_computer_status(computer_status::online);
}

logger_ptr base_distributed_task::create_logger() const {
  auto l_logger_path = core_set::get_set().get_cache_root() / server_task_info::logger_category /
                       fmt::format("{}.log", task_info_.uuid_id_);
  auto l_logger = std::make_shared<spdlog::async_logger>(
      task_info_.name_, std::make_shared<spdlog::sinks::basic_file_sink_mt>(l_logger_path.generic_string()),
      spdlog::thread_pool()
  );
  l_logger->sinks().push_back(
      std::make_shared<run_ue_assembly_distributed_sink_mt>(create_kitsu_client(), task_info_.uuid_id_)
  );
  return l_logger;
}

std::shared_ptr<kitsu::kitsu_client> base_distributed_task::create_kitsu_client() const {
  auto l_client = std::make_shared<kitsu::kitsu_client>(core_set::get_set().server_ip);
  l_client->set_token(token_);
  return l_client;
}

}  // namespace doodle::http