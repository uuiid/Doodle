//
// Created by td_main on 2023/8/4.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <nlohmann/json.hpp>
namespace doodle::render_farm {
namespace detail {
class render_ue4 {
 public:
  // 此处对于ue4的渲染器的参数进行了封装
  struct arg {
   public:
    std::string ProjectPath;
    std::string Args;
    std::string ManifestValue;

    std::string out_file_path;

    // to json
    friend void to_json(nlohmann::json& j, const arg& in_arg) {
      j["projectPath"]   = in_arg.ProjectPath;
      j["args"]          = in_arg.Args;
      j["manifestValue"] = in_arg.ManifestValue;
      j["outFilePath"]   = in_arg.out_file_path;
    }

    // form json
    friend void from_json(const nlohmann::json& j, arg& in_arg) {
      j.at("projectPath").get_to(in_arg.ProjectPath);
      j.at("args").get_to(in_arg.Args);
      j.at("manifestValue").get_to(in_arg.ManifestValue);
      j.at("outFilePath").get_to(in_arg.out_file_path);
    }
  };

  explicit render_ue4(entt::handle self_handle, arg in_arg)
      : arg_(std::move(in_arg)), self_handle_(std::move(self_handle)) {
    set_meg();
  }
  ~render_ue4() = default;
  void run();

 private:
  arg arg_;
  entt::handle self_handle_;
  FSys::path manifest_path_;
  void set_meg();

  // 下载文件
  bool download_file(const FSys::path& in_file_path);
  // 生成命令行
  [[nodiscard("")]] std::string generate_command_line() const;
  void run_impl(bool in_r);
};

}  // namespace detail
using render_ue4_ptr = std::shared_ptr<detail::render_ue4>;
}  // namespace doodle::render_farm
