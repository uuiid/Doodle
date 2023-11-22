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
  std::string maya_out_data_{};
  entt::handle msg_{};
  boost::asio::any_io_executor executor_{};

  struct data_t {
    FSys::path render_project_{};
    FSys::path render_project_file_{};
    std::string render_map_{};
  };
  std::unique_ptr<data_t> data_{};

  void down_file(const FSys::path& in_path, bool is_scene) const;
  void render() const;
  FSys::path gen_render_config_file() const;
  FSys::path write_python_script() const;

 public:
  explicit maya_to_exe_file(std::string in_maya_out_data)
      : maya_out_data_(std::move(in_maya_out_data)), data_(std::make_unique<data_t>()) {
    msg_      = {*g_reg(), g_reg()->create()};
    executor_ = boost::asio::make_strand(g_thread());
  };
  virtual ~maya_to_exe_file() = default;

  void operator()(boost::system::error_code in_error_code) const;
};

}  // namespace doodle
