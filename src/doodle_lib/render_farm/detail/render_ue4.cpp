//
// Created by td_main on 2023/8/4.
//

#include "render_ue4.h"

#include <doodle_lib/exe_warp/ue_exe.h>
namespace doodle::render_farm {
namespace detail {

void render_ue4::run() {
  if (download_file("D:/doodle/cache/ue")) {
    doodle_lib::Get().ctx().emplace<ue_exe>().async_run(
        self_handle_, ue_exe::arg_render_queue{generate_command_line()}, [](auto&&) {}
    );
  }
}
bool render_ue4::download_file(const FSys::path& in_file_path) {
  constexpr auto g_config     = "Config";
  constexpr auto g_content    = "Content";

  FSys::path const l_prj_path = arg_.ProjectPath;
  if (!FSys::exists(l_prj_path)) {
    return false;
  }
  if (!FSys::exists(in_file_path)) FSys::create_directories(in_file_path);

  const auto l_loc = in_file_path / l_prj_path.filename();
  {  // 复制项目文件
    FSys::copy(l_prj_path, l_loc, FSys::copy_options::overwrite_existing);
  }
  {
    // 复制配置文件夹
    FSys::copy(
        l_prj_path.parent_path() / g_config, l_loc.parent_path() / g_config,
        FSys::copy_options::overwrite_existing | FSys::copy_options::recursive
    );
  }
  {
    // 复制内容文件夹
    auto l_loc_content = l_loc.parent_path() / g_content;
    auto l_prj_content = l_prj_path.parent_path() / g_content;
    for (auto&& i : FSys::recursive_directory_iterator{l_prj_content}) {
      auto l_loc_ = l_loc_content / i.path().lexically_relative(l_prj_content);
      if (i.file_size() != FSys::file_size(l_loc_) || i.last_write_time() != FSys::last_write_time(l_loc_)) {
        FSys::copy(
            i.path(), l_loc.parent_path() / g_content / i.path().filename(), FSys::copy_options::overwrite_existing
        );
      }
    }
  }

  arg_.ProjectPath = l_loc.lexically_normal().generic_string();

  {
    // 写入数据
    manifest_path_ =
        l_loc.parent_path() / "Saved" / "MovieRenderPipeline" / fmt::format("{}.txt", core_set::get_set().get_uuid());
    std::ofstream l_ofs{manifest_path_};
    l_ofs << arg_.ManifestValue;
  }
  return true;
}
std::string render_ue4::generate_command_line() const {
  return fmt::format(
      R"({} {} -MoviePipelineConfig="{}")", arg_.ProjectPath, arg_.Args, manifest_path_.generic_string()
  );
}
}  // namespace detail
}  // namespace doodle::render_farm