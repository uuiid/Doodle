//
// Created by TD on 2024/2/20.
//

#pragma once
#include <cstdint>
#include <string_view>

namespace doodle::http {
/**
 * @brief 共享内存中保存的监听端口信息
 *
 * 固定大小(8 字节), 便于用 shared_memory_object + truncate 直接覆盖写,
 * 不需要 managed_shared_memory 的分配器与段内元数据。
 */
struct http_listener_port_shm_t {
  /// 魔数, 读端用于确认这块内存确实是本结构, 而不是残留的脏数据
  std::uint32_t magic_;
  /// 结构版本, 为以后扩展字段留位
  std::uint32_t version_;
  /// 实际监听端口(内核分配的真实值, 不是入参 0)
  std::uint16_t port_;
  /// 对齐填充, 保持结构 8 字节
  std::uint16_t reserved_;
};

/// 魔数取值, 任意固定常量即可
constexpr std::uint32_t g_listener_port_shm_magic{0x444C'5054U};  // "DLPT"
/// 结构版本
constexpr std::uint32_t g_listener_port_shm_version{1};

/**
 * @brief 固定共享内存的名字
 *
 * 全局唯一, 不做实例区分: 同一台机器上后启动的监听器直接覆盖先前的端口。
 * 只含 ASCII 字母数字下划线, 在 Windows 命名对象下合法。
 */
constexpr std::string_view g_listener_port_shm_name{"doodle_http_listener_port"};

/**
 * @brief 把监听端口写入固定共享内存
 *
 * 已存在则原地覆盖, 无需先删除。本函数**不抛异常**:
 * 任何失败(权限/名字冲突/truncate 失败等)都只记 err 日志并返回 false,
 * 调用方(监听协程)必须照常继续 accept, 不能因发布失败而拒绝服务。
 *
 * @param in_port 实际监听端口
 * @return 写入成功返回 true
 */
bool publish_listener_port(std::uint16_t in_port);

/**
 * @brief 从固定共享内存读取监听端口
 *
 * 会校验魔数与版本号, 因此从未写入过或内容损坏时返回 false。
 *
 * @param out_port 读取成功时写入该参数
 * @return 读取且校验通过返回 true
 */
bool read_listener_port(std::uint16_t& out_port);

}  // namespace doodle::http
