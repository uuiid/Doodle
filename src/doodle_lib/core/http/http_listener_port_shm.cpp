//
// Created by TD on 2024/2/20.
//

#include "http_listener_port_shm.h"

#include <doodle_lib/logger/logger.h>

#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/process/v2/pid.hpp>

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

std::string shm_name() {
  return std::string{g_listener_port_shm_name} + "_" + std::to_string(boost::process::v2::current_pid());
}

constexpr std::size_t g_shm_size{sizeof(http_listener_port_shm_t)};
}  // namespace

bool publish_listener_port(std::uint16_t in_port) {
  try {
    auto l_name    = shm_name();
    auto& l_handle = publish_handle();
    if (!l_handle.get_name()) {
      l_handle = windows_shared_memory{
          boost::interprocess::open_or_create,
          l_name.c_str(),
          boost::interprocess::read_write,
          static_cast<std::uint32_t>(g_shm_size)
      };
    }
    mapped_region l_region{l_handle, boost::interprocess::read_write};
    if (l_region.get_size() < g_shm_size) {
      default_logger_raw()->log(log_loc(), level::err, "共享内存 {} 尺寸不足, 无法写入监听端口", l_name);
      return false;
    }

    http_listener_port_shm_t l_data{
        .magic_ = g_listener_port_shm_magic, .version_ = g_listener_port_shm_version, .port_ = in_port, .reserved_ = 0
    };
    std::memcpy(l_region.get_address(), &l_data, sizeof(l_data));
    return true;
  } catch (const std::exception& in_err) {
    default_logger_raw()->log(log_loc(), level::err, "写入共享内存中的监听端口失败: {}", in_err.what());
    return false;
  } catch (...) {
    default_logger_raw()->log(log_loc(), level::err, "写入共享内存中的监听端口失败: 未知错误");
    return false;
  }
}

}  // namespace doodle::http
