//
// Created by TD on 2021/12/25.
//

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle::details {
class DOODLELIB_API ue4_exe : public process_t<ue4_exe> {
 public:
  using base_type = process_t<ue4_exe>;

  /**
   * @brief 使用 ue4 运行任意命令
   * @param in_handle 具有消息组件的的句柄
   * @param in_file 传入的命令(不要包括 ue4.exe)
   */
  explicit ue4_exe(const entt::handle &in_handle, const string &in_file);
  /**
   * @brief 创建ue4 关卡序列
   * @param in_handle 具有消息组件的的句柄
   * @param in_file 具有 project 和 shot episodes 组件的句柄
   *
   */
  explicit ue4_exe(const entt::handle &in_handle, const std::vector<entt::handle> &in_file);

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(base_type::delta_type, void *data);
};
}  // namespace doodle::details
