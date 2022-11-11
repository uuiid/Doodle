//
// Created by TD on 2022/10/13.
//

#include "maya_create_movie.h"

#include "doodle_core/logger/logger.h"

#include <doodle_app/app/this_rpc_exe.h>

namespace doodle::maya_plug::detail {

class maya_create_movie::impl {
 public:
  doodle::detail::this_rpc_exe doodle_exe_attr;
};

maya_create_movie::maya_create_movie() : ptr(std::make_unique<impl>()) {}

void maya_create_movie::create_move(
    const FSys::path& in_out_path, process_message& in_msg, const std::vector<image_attr>& in_vector
) {
  DOODLE_LOG_INFO("开始doodle 进程合成视频");
  ptr->doodle_exe_attr.create_move(in_out_path, in_vector, in_msg);
  ptr->doodle_exe_attr.wait();
}
FSys::path maya_create_movie::create_out_path(const entt::handle& in_handle) {
  boost::ignore_unused(this);

  FSys::path l_out{};
  l_out = in_handle.get<FSys::path>();

  /// \brief 这里我们检查 shot，episode 进行路径的组合
  if (!l_out.has_extension() && in_handle.any_of<episodes, shot>())
    l_out /= fmt::format(
        "{}_{}.mp4", in_handle.any_of<episodes>() ? fmt::to_string(in_handle.get<episodes>()) : "eps_none"s,
        in_handle.any_of<shot>() ? fmt::to_string(in_handle.get<shot>()) : "sh_none"s
    );
  else if (!l_out.has_extension()) {
    l_out /= fmt::format("{}.mp4", core_set::get_set().get_uuid());
  } else
    l_out.extension() == ".mp4" ? void() : throw_exception(doodle_error{"扩展名称不是MP4"});

  if (exists(l_out.parent_path())) create_directories(l_out.parent_path());
  return l_out;
}
maya_create_movie::~maya_create_movie() = default;
}  // namespace doodle::maya_plug::detail