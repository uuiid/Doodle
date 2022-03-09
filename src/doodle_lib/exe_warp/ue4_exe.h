//
// Created by TD on 2021/12/25.
//

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {
namespace ue4_comm {
class ue4_comm_str {
 public:
  std::string comm;
};

class DOODLELIB_API sequencer_comm : public process_t<sequencer_comm> {
 private:

 public:
  using base_type = process_t<sequencer_comm>;
  /**
   * @brief 生成ue4命令，
   * @param in_handle 具有消息组件的句柄
   * @param in_vector 传入的具有 episodes 和 shot 组件的句柄列表
   */
  explicit sequencer_comm(const entt::handle &in_handle, std::vector<entt::handle> &in_vector);
  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const base_type::delta_type &, void *data);
};

class DOODLELIB_API import_file_comm : public process_t<import_file_comm> {
 public:
  using base_type = process_t<import_file_comm>;
  /**
   * @brief 生成ue4命令，
   * @param in_handle 具有消息组件的句柄
   * @param in_vector 传入的具有 episodes 和 shot 以及 assets_file 组件的句柄列表
   */
  explicit import_file_comm(const entt::handle &in_handle, std::vector<entt::handle> &in_vector);
  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const base_type::delta_type &, void *data);
};
}  // namespace ue4_comm

/**
 * @brief
 *
 * @note
 *  * 查看 FSequencer::CreateCamera() 和 MovieSceneToolHelpers::CreateCameraCutSectionForCamera 以及 UMovieScene::AddCameraCutTrack 函数进行创建ue4 定序器 camera
 *  *
 */
class DOODLELIB_API ue4_exe : public process_t<ue4_exe> {
 public:
  using base_type = process_t<ue4_exe>;

  class DOODLELIB_API import_file {
   public:
    entt::handle handle_;
    FSys::path in_path;
  };

  /**
   * @brief 使用 ue4 运行任意命令
   * @param in_handle 具有消息组件 和 命令组件 的句柄
   */
  explicit ue4_exe(const entt::handle &in_handle);
  /**
   * @brief 使用 ue4 运行任意命令
   * @param in_handle 具有消息组件的的句柄
   * @param in_comm 传入的命令(不要包括 ue4.exe)
   */
  explicit ue4_exe(const entt::handle &in_handle, const string &in_comm);

  /**
   * @brief 使用 ue4 运行任意命令
   * @param in_handle 具有消息组件的的句柄
   * @param in_comm 传入的命令(不要包括 ue4.exe)
   */
  explicit ue4_exe(const entt::handle &in_handle, const std::shared_ptr<string> &in_comm);
  /**
   * @brief 创建ue4 关卡序列
   * @param in_handle 具有消息组件 和 project 组件的句柄
   * @param in_vector 传入的导入文件的类列表
   */
  explicit ue4_exe(const entt::handle &in_handle, const std::vector<import_file> &in_vector);

  /**
   * @brief 导入文件的命令
   * @param in_handle 具有消息组件 和 project 和
   * @param in_vector
   */
  //  explicit ue4_exe(const entt::handle &in_handle, const std::vector<entt::handle> &in_vector);

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const base_type::delta_type &, void *data);
};
}  // namespace doodle
