//
// Created by TD on 2023/11/17.
//

#include "maya_to_exe_file.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/thread_pool/process_message.h>
namespace doodle {

namespace maya_to_exe_file_ns {
struct maya_out_arg {
  FSys::path out_file{};
  // 引用文件
  FSys::path ref_file{};
  friend void from_json(const nlohmann::json &nlohmann_json_j, maya_out_arg &nlohmann_json_t) {
    nlohmann_json_j["out_file"].get_to(nlohmann_json_t.out_file);
    nlohmann_json_j["ref_file"].get_to(nlohmann_json_t.ref_file);
  };
};
}  // namespace maya_to_exe_file_ns

void maya_to_exe_file::operator()(boost::system::error_code in_error_code) const {
  if (in_error_code) {
    log_error(fmt::format("maya_to_exe_file error:{}", in_error_code));
    return;
  }
  if (maya_out_data_.empty()) {
    auto &l_msg = msg_.get<process_message>();
    l_msg.message("maya结束进程后未能成功输出文件");
    l_msg.set_state(l_msg.fail);
    return;
  }

  auto l_id_map_tmp = g_reg()->view<database>();
  auto l_id_map     = l_id_map_tmp | ranges::views::transform([](const entt::entity &in_entity) {
                    return std::make_pair(g_reg()->get<database>(in_entity).uuid(), in_entity);
                  }) |
                  ranges::to<std::unordered_map<uuid, entt::entity>>();

  auto l_maya_out_arg = nlohmann::json ::parse(maya_out_data_).get<std::vector<maya_to_exe_file_ns::maya_out_arg>>();
  auto l_refs =
      l_maya_out_arg |
      ranges::views::transform([](const maya_to_exe_file_ns::maya_out_arg &in_arg) { return in_arg.ref_file; }) |
      ranges::views::transform([&](const FSys::path &in_arg) -> entt::handle {
        auto l_uuid = FSys::software_flag_file(in_arg);

        if (l_uuid.is_nil()) return entt::handle{};

        if (l_id_map.contains(l_uuid)) {
          return entt::handle{*g_reg(), l_id_map.at(l_uuid)};
        } else
          return entt::handle{};
      }) |
      ranges::to<std::vector<entt::handle>>();

  if (l_refs.empty()) {
    return;
  }
}
}  // namespace doodle