//
// Created by TD on 2022/1/20.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API get_input_dialog : public process_t<get_input_dialog> {
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
  virtual void render() = 0;

 public:
  explicit get_input_dialog();
  ~get_input_dialog() override;
  [[maybe_unused]] virtual void init();
  [[maybe_unused]] virtual void succeeded();
  [[maybe_unused]] virtual void failed();
  [[maybe_unused]] virtual void aborted();
  [[maybe_unused]] virtual void update(delta_type, void* data);
};

class DOODLELIB_API get_input_project_dialog : public get_input_dialog {
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
  void render() override;

 public:
  explicit get_input_project_dialog(std::shared_ptr<FSys::path> in_handle);
  ~get_input_project_dialog() override;
  virtual void init();
  virtual void succeeded();
  virtual void failed();
  virtual void aborted();
};

namespace gui::input {
class DOODLELIB_API get_bool_dialog : public get_input_dialog {
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
  void render() override;

 public:
  explicit get_bool_dialog(const std::shared_ptr<bool>& is_quit);
};
}  // namespace gui::input
}  // namespace doodle
