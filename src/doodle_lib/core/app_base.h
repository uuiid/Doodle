//
// Created by TD on 2022/1/18.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/platform/win/windows_alias.h>
namespace doodle {
class DOODLELIB_API app_base {
 protected:
  static app_base* self;

  std::wstring p_title;
  std::atomic_bool stop_;
  win::wnd_instance instance;
  doodle_lib_ptr p_lib;
  program_options_ptr options_;

 public:
  explicit app_base();
  explicit app_base(const win::wnd_instance& in_instance);
  virtual ~app_base();

  inline void command_line_parser(int argc, char* argv[]) {
    std::vector<std::string> l_str{argv, argv + argc};
    command_line_parser(l_str);
  };
  void command_line_parser(const LPSTR& in_arg);
  virtual void command_line_parser(const std::vector<string>& in_arg);

  /**
   * @brief 直接使用默认配置运行
   * @return
   */
  virtual std::int32_t run();
  virtual void loop_one() = 0;
  std::atomic_bool& stop();
  virtual bool valid() const;

  DOODLE_DIS_COPY(app_base);
  static app_base& Get();
};

class DOODLELIB_API app_command_base : public app_base {
 public:
  void loop_one() override;
  static app_command_base& Get();
};

}  // namespace doodle
