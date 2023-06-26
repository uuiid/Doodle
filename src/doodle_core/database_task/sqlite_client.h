//
// Created by TD on 2022/6/2.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>

#include <boost/asio/async_result.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

#include <filesystem>
#include <memory>
#include <utility>
namespace doodle::database_n {

class file_translator;
using file_translator_ptr = std::shared_ptr<file_translator>;
class DOODLE_CORE_API file_translator : public std::enable_shared_from_this<file_translator> {
 private:
  bool is_run{};
  class impl;
  std::unique_ptr<impl> ptr;

 protected:
  FSys::path project_path;

  virtual void async_open_impl(
      const FSys::path& in_path, const std::shared_ptr<std::function<void(bsys::error_code)>>& in_call
  );
  virtual void async_save_impl(const std::shared_ptr<std::function<void(bsys::error_code)>>& in_call);
  virtual void async_import_impl(
      const FSys::path& in_path, const std::shared_ptr<std::function<void(bsys::error_code)>>& in_call
  );
  using call_error     = std::function<void(bsys::error_code)>;
  using call_error_ptr = std::shared_ptr<call_error>;

 public:
  file_translator();
  explicit file_translator(registry_ptr in_registry);
  ~file_translator();
  /**
   * @brief 使用路径打开项目文件
   * @param in_path 传入的项目文件路径
   */
  template <typename CompletionToken>
  auto async_open(const FSys::path& in_path, CompletionToken&& token) ->
      typename boost::asio::async_result<typename std::decay_t<CompletionToken>, void(bsys::error_code)>::return_type {
    return boost::asio::async_initiate<CompletionToken, void(bsys::error_code)>(
        [in_path, l_s = this->shared_from_this()](auto&& completion_handler) {
          l_s->async_open_impl(
              in_path, std::make_shared<call_error>(std::forward<decltype(completion_handler)>(completion_handler))
          );
        },
        token
    );
  };

  /**
   * @brief 使用路径打开项目文件
   * @param in_path 传入的项目文件路径
   */
  template <typename CompletionToken>
  auto async_save(CompletionToken&& token) ->
      typename boost::asio::async_result<typename std::decay_t<CompletionToken>, void(bsys::error_code)>::return_type {
    return boost::asio::async_initiate<CompletionToken, void(bsys::error_code)>(
        [l_s = this->shared_from_this()](auto&& completion_handler) {
          l_s->async_save_impl(
              std::make_shared<call_error>(std::forward<decltype(completion_handler)>(completion_handler))
          );
        },
        token
    );
  };
  /**
   * @brief 使用路径打开项目文件
   * @param in_path 传入的项目文件路径
   */
  template <typename CompletionToken>
  auto async_import(const FSys::path& in_path, CompletionToken&& token) ->
      typename boost::asio::async_result<typename std::decay_t<CompletionToken>, void(bsys::error_code)>::return_type {
    return boost::asio::async_initiate<CompletionToken, void(bsys::error_code)>(
        [in_path, l_s = this->shared_from_this()](auto&& completion_handler) {
          l_s->async_import_impl(
              in_path, std::make_shared<call_error>(std::forward<decltype(completion_handler)>(completion_handler))
          );
        },
        token
    );
  };
  inline void save_(const FSys::path& in_path) { project_path = in_path; }
  inline void open_(const FSys::path& in_path) { project_path = in_path; }

  virtual void new_file_scene(const FSys::path& in_path, const project& in_project);
  inline FSys::path get_project_path() const { return project_path; }
};

}  // namespace doodle::database_n
