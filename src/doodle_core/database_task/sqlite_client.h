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
  bool is_run{};
  std::any obs{};
  registry_ptr registry_attr{};
  std::atomic_bool save_all{};

 protected:
  FSys::path project_path;

  virtual void async_open_impl(const FSys::path& in_path);
  virtual void async_save_impl();
  virtual void async_import_impl(const FSys::path& in_path);

 public:
  file_translator();
  explicit file_translator(registry_ptr in_registry);
  ~file_translator() = default;
  /**
   * @brief 使用路径打开项目文件
   * @param in_path 传入的项目文件路径
   */

  inline auto async_open(const FSys::path& in_path) { return async_open_impl(in_path); };

  /**
   * @brief 使用路径打开项目文件
   * @param in_path 传入的项目文件路径
   */

  inline auto async_save() { return async_save_impl(); };
  /**
   * @brief 使用路径打开项目文件
   * @param in_path 传入的项目文件路径
   */

  inline auto async_import(const FSys::path& in_path) { return async_import_impl(in_path); };

  virtual void new_file_scene(const FSys::path& in_path, const project& in_project);
  inline FSys::path get_project_path() const { return project_path; }
};

}  // namespace doodle::database_n
