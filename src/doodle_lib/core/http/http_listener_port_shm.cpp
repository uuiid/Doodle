//
// Created by TD on 2024/2/20.
//

#include "http_listener_port_shm.h"

#include <doodle_lib/logger/logger.h>

#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/windows_shared_memory.hpp>

#include <cstdint>
#include <cstring>
#include <string>

namespace doodle::http {
namespace {
using windows_shared_memory = boost::interprocess::windows_shared_memory;
using mapped_region         = boost::interprocess::mapped_region;

/**
 * @brief 发布端持有的命名段句柄
 *
 * windows_shared_memory 是 Windows 原生命名对象(CreateFileMappingW), 最后一个 attach 的进程
 * 退出时内核自动销毁段; 若只让局部变量持有, publish 返回后段就没了, 读端必然拿不到。
 * 因此句柄必须活到进程结束。
 */
windows_shared_memory& publish_handle() {
  static windows_shared_memory l_handle{};
  return l_handle;
}

constexpr std::size_t g_shm_size{sizeof(http_listener_port_shm_t)};
}  // namespace

bool publish_listener_port(std::uint16_t in_port) {
  try {
    auto& l_handle = publish_handle();
    if (!l_handle.get_name()) {
      l_handle = windows_shared_memory{
          boost::interprocess::open_or_create,
          std::string{g_listener_port_shm_name}.c_str(),
          boost::interprocess::read_write,
          static_cast<std::uint32_t>(g_shm_size)
      };
    }
    mapped_region l_region{l_handle, boost::interprocess::read_write};
    if (l_region.get_size() < g_shm_size) {
      default_logger_raw()->log(log_loc(), level::err, "共享内存 {} 尺寸不足, 无法写入监听端口", g_listener_port_shm_name);
      return false;
    }

    http_listener_port_shm_t l_data{
        .magic_ = g_listener_port_shm_magic, .version_ = g_listener_port_shm_version, .port_ = in_port, .reserved_ = 0
    };
    std::memcpy(l_region.get_address(), &l_data, sizeof(l_data));
    return true;
  } catch (const std::exception& in_err) {
    default_logger_raw()->log(
        log_loc(), level::err, "写入共享内存 {} 中的监听端口失败: {}", g_listener_port_shm_name, in_err.what()
    );
    return false;
  } catch (...) {
    default_logger_raw()->log(
        log_loc(), level::err, "写入共享内存 {} 中的监听端口失败: 未知错误", g_listener_port_shm_name
    );
    return false;
  }
}

std::optional<std::uint16_t> read_listener_port() {
  try {
    windows_shared_memory l_shm{
        boost::interprocess::open_only, std::string{g_listener_port_shm_name}.c_str(), boost::interprocess::read_only
    };
    mapped_region l_region{l_shm, boost::interprocess::read_only};
    if (l_region.get_size() < g_shm_size) return std::nullopt;

    http_listener_port_shm_t l_data{};
    std::memcpy(&l_data, l_region.get_address(), sizeof(l_data));
    if (l_data.magic_ != g_listener_port_shm_magic) return std::nullopt;
    if (l_data.version_ != g_listener_port_shm_version) return std::nullopt;

    return l_data.port_;
  } catch (const std::exception& in_err) {
    default_logger_raw()->log(
        log_loc(), level::err, "读取共享内存 {} 中的监听端口失败: {}", g_listener_port_shm_name, in_err.what()
    );
    return std::nullopt;
  } catch (...) {
    default_logger_raw()->log(
        log_loc(), level::err, "读取共享内存 {} 中的监听端口失败: 未知错误", g_listener_port_shm_name
    );
    return std::nullopt;
  }
}

}  // namespace doodle::http
