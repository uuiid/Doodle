//
// Created by TD on 2023/11/17.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio.hpp>

namespace doodle {

class maya_to_exe_file {
 private:
  // maya 输出结果文件内容
  FSys::path maya_out_file_{};
  FSys::path update_dir_{};  // 传入的上传路径, 我们会对其进行组合
  entt::handle msg_{};
  boost::asio::any_io_executor executor_{};
  static constexpr auto g_config        = "Config";
  static constexpr auto g_content       = "Content";
  static constexpr auto g_uproject      = ".uproject";
  static constexpr auto g_saved         = "Saved";
  static constexpr auto g_movie_renders = "MovieRenders";

  struct copy_file_strand {
    boost::asio::strand<boost::asio::thread_pool::executor_type> executor_{};
  };

  enum class render_type {
    down_file,
    import_file,
    render,
    update,
  };

  struct data_t {
    logger_ptr logger_{};
    render_type render_type_{render_type::down_file};
    std::string maya_out_data_{};
    FSys::path render_project_{};
    FSys::path render_project_file_{};
    std::string import_dir_{};  // 导入路径

    std::string render_map_{};       // 渲染生成的主管卡(包括灯光, 场景等)
    std::string original_map_{};     // 原始的主管卡
    std::string render_config_{};    // 渲染配置文件
    std::string create_map_{};       // 渲染生成关卡(只包含导入的文件)
    std::string render_sequence_{};  // 渲染序列文件
    FSys::path out_dir{};
    FSys::path update_dir_{};  // 真正的上传文件路径

    std::vector<FSys::path> extra_update_dir_{};  // 额外添加的上传路径

    boost::asio::any_completion_handler<void(boost::system::error_code)> end_call_{};
  };
  std::shared_ptr<data_t> data_{};

  void call_end(const boost::system::error_code& in_error_code) const;
  void down_file(const FSys::path& in_path, bool is_scene) const;
  void import_file() const;
  void render(boost::system::error_code in_error_code) const;
  void update_file(boost::system::error_code in_error_code) const;

  void begin_render(boost::system::error_code in_error_code) const;
  [[nodiscard]] FSys::path gen_render_config_file() const;

 public:
  explicit maya_to_exe_file(entt::handle in_msg, std::string in_maya_out_data, FSys::path in_update_path)
      : data_(std::make_shared<data_t>()) {
    data_->logger_        = in_msg.get<process_message>().logger();
    data_->maya_out_data_ = std::move(in_maya_out_data);
    update_dir_           = std::move(in_update_path);
    msg_                  = std::move(in_msg);
    g_ctx().emplace<copy_file_strand>(boost::asio::make_strand(g_thread()));
  };
  explicit maya_to_exe_file(entt::handle in_msg, FSys::path in_maya_out_file, FSys::path in_update_path)
      : maya_out_file_(std::move(in_maya_out_file)), data_(std::make_shared<data_t>()) {
    data_->logger_ = in_msg.get<process_message>().logger();
    msg_           = std::move(in_msg);
    update_dir_    = std::move(in_update_path);
    g_ctx().emplace<copy_file_strand>(boost::asio::make_strand(g_thread()));
  };
  template <typename CompletionHandler>
  inline maya_to_exe_file& set_ue_call_fun(CompletionHandler in_end_call_) {
    executor_        = boost::asio::get_associated_executor(in_end_call_);
    data_->end_call_ = std::move(in_end_call_);
    return *this;
  }

  virtual ~maya_to_exe_file() = default;

  void operator()(boost::system::error_code in_error_code) const;
  void operator()() const;
  void operator()(const FSys::path& in_path, bool is_scene) const;
};

}  // namespace doodle
