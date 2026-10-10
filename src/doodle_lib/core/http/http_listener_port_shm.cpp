//
// Created by TD on 2024/2/20.
//

#include "http_listener_port_shm.h"

#include <doodle_lib/logger/logger.h>

#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include <cstring>
#include <string>

namespace doodle::http {
namespace {
using shared_memory_object = boost::interprocess::shared_memory_object;
using mapped_region        = boost::interprocess::mapped_region;
// Boost 1.90: offset_t 不再是 shared_memory_object 的嵌套类型, 改由
// boost/interprocess/detail/os_file_functions.hpp 在 boost::interprocess 作用域直接提供
// (Windows 分支 typedef __int64 offset_t)。写 shared_memory_object::offset_t 会 C2039。
using offset_t = boost::interprocess::offset_t;
}  // namespace

bool publish_listener_port(std::uint16_t in_port) {
  try {
    // open_or_create: 上次运行残留的段会被直接复用, 无需先 remove
    shared_memory_object l_shm{
        boost::interprocess::open_or_create, std::string{g_listener_port_shm_name}.c_str(),
        boost::interprocess::read_write
    };
    // 先缩到 0 再扩到目标大小, 保证重复发布/结构变更后内容干净
    l_shm.truncate(0);
    l_shm.truncate(static_cast<offset_t>(sizeof(http_listener_port_shm_t)));

    mapped_region l_region{l_shm, boost::interprocess::read_write};
    if (l_region.get_size() < sizeof(http_listener_port_shm_t)) {
      default_logger_raw()->log(
          log_loc(), level::err, "共享内存 {} 尺寸不足, 无法写入监听端口", g_listener_port_shm_name
      );
      return false;
    }

    http_listener_port_shm_t l_data{
        .magic_ = g_listener_port_shm_magic, .version_ = g_listener_port_shm_version, .port_ = in_port, .reserved_ = 0
    };
    std::memcpy(l_region.get_address(), &l_data, sizeof(l_data));
    return true;
  } catch (const std::exception& in_err) {
    // 发布失败不能影响监听, 只记日志
    default_logger_raw()->log(log_loc(), level::err, "写入共享内存 {} 中的监听端口失败: {}", g_listener_port_shm_name, in_err.what());
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
    // open_only: 只读取, 不产生副作用, 也不会创建段
    shared_memory_object l_shm{
        boost::interprocess::open_only, std::string{g_listener_port_shm_name}.c_str(), boost::interprocess::read_only
    };
    mapped_region l_region{l_shm, boost::interprocess::read_only};
    if (l_region.get_size() < sizeof(http_listener_port_shm_t)) return std::nullopt;

    http_listener_port_shm_t l_data{};
    std::memcpy(&l_data, l_region.get_address(), sizeof(l_data));
    // 校验魔数与版本, 挡住从未写入过或已损坏的内容
    if (l_data.magic_ != g_listener_port_shm_magic) return std::nullopt;
    if (l_data.version_ != g_listener_port_shm_version) return std::nullopt;

    return l_data.port_;
  } catch (const std::exception& in_err) {
    default_logger_raw()->log(log_loc(), level::err, "读取共享内存 {} 中的监听端口失败: {}", g_listener_port_shm_name, in_err.what());
    return std::nullopt;
  } catch (...) {
    default_logger_raw()->log(
        log_loc(), level::err, "读取共享内存 {} 中的监听端口失败: 未知错误", g_listener_port_shm_name
    );
    return std::nullopt;
  }
}

}  // namespace doodle::http
