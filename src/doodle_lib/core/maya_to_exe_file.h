//
// Created by TD on 2023/11/17.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio.hpp>

namespace doodle {

class maya_to_exe_file {
 private:
  // maya 输出结果文件内容
  FSys::path maya_out_file_{};
  entt::handle msg_{};
  boost::asio::any_io_executor executor_{};

  struct data_t {
    std::string maya_out_data_{};
    FSys::path render_project_{};
    FSys::path render_project_file_{};
    std::string render_map_{};
  };
  std::shared_ptr<data_t> data_{};

  void down_file(const FSys::path& in_path, bool is_scene) const;
  void render() const;
  [[nodiscard]] FSys::path gen_render_config_file() const;
  [[nodiscard]] FSys::path write_python_script() const;

 public:
  explicit maya_to_exe_file(entt::handle in_msg, std::string in_maya_out_data) : data_(std::make_shared<data_t>()) {
    data_->maya_out_data_ = std::move(in_maya_out_data);
    msg_                  = {*g_reg(), g_reg()->create()};
    executor_             = boost::asio::make_strand(g_thread());
  };
  explicit maya_to_exe_file(entt::handle in_msg, FSys::path in_maya_out_file)
      : maya_out_file_(std::move(in_maya_out_file)), data_(std::make_shared<data_t>()) {
    msg_      = {*g_reg(), g_reg()->create()};
    executor_ = boost::asio::make_strand(g_thread());
  };

  virtual ~maya_to_exe_file() = default;

  void operator()(boost::system::error_code in_error_code) const;
};

}  // namespace doodle
