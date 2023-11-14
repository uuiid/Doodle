//
// Created by TD on 2022/6/2.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <any>
#include <filesystem>
#include <memory>
#include <utility>
namespace doodle::database_n {

class file_translator;
using file_translator_ptr = std::shared_ptr<file_translator>;
class DOODLE_CORE_API file_translator : public std::enable_shared_from_this<file_translator> {
 private:
  using timer_t = boost::asio::steady_timer;

  std::any obs{};
  registry_ptr registry_attr{};
  std::atomic_bool save_all{};
  bool only_ctx{};
  bool only_open{};

  std::shared_ptr<timer_t> timer_{};  // 定时器

  void begin_save();

 protected:
  FSys::path project_path;

  virtual void async_open_impl(const FSys::path& in_path);
  virtual void async_save_impl();
  virtual void async_import_impl(const FSys::path& in_path);

 public:
  file_translator();
  explicit file_translator(registry_ptr in_registry);
  ~file_translator() = default;

  inline void set_only_ctx(bool in_only_ctx) { only_ctx = in_only_ctx; }
  inline void set_only_open(bool in_only_open) { only_open = in_only_open; }
  /**
   * @brief 使用路径打开项目文件
   * @param in_path 传入的项目文件路径
   */

  inline auto async_open(const FSys::path& in_path) {
    return boost::asio::post(g_io_context(), [this, in_path]() { async_open_impl(in_path); });
  };

  /**
   * @brief 使用路径打开项目文件
   * @param in_path 传入的项目文件路径
   */

  inline auto async_import(const FSys::path& in_path) { return async_import_impl(in_path); };

  virtual void new_file_scene(const FSys::path& in_path, const project& in_project);
  inline FSys::path get_project_path() const { return project_path; }
};

}  // namespace doodle::database_n
