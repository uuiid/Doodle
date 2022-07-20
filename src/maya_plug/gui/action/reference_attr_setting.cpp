//
// Created by TD on 2021/10/14.
//

#include "reference_attr_setting.h"

#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_core/thread_pool/process_pool.h>
#include <doodle_lib/gui/gui_ref/ref_base.h>

#include <maya/MTime.h>
#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MUuid.h>
#include <maya/adskDataAssociations.h>
#include <maya/adskDataStream.h>
#include <maya/adskDebugPrint.h>
#include <maya/MNamespace.h>

#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/maya_plug_fwd.h>
#include <maya_plug/data/sim_cover_attr.h>

namespace doodle {
namespace maya_plug {

namespace reference_attr {

bool data::operator==(const data& in_rhs) const {
  return path == in_rhs.path;
}
bool data::operator!=(const data& in_rhs) const {
  return !(in_rhs == *this);
}
}  // namespace reference_attr

class reference_attr_setting::impl {
 public:
  std::vector<entt::handle> p_handles;
  entt::handle p_current_select;

  ::doodle::gui::gui_cache<bool> simple_subsampling{"simple subsampling", true};
  ::doodle::gui::gui_cache<std::int32_t> frame_samples{"frame samples"s, 6};
  ::doodle::gui::gui_cache<std::float_t> time_scale{"time scale"s, 1.0f};
  ::doodle::gui::gui_cache<std::float_t> length_scale{"length scale"s, 1.0f};
  ::doodle::gui::gui_cache<std::int32_t> max_cg_iteration{"max CGIteration"s, 1000};
  ::doodle::gui::gui_cache<std::int32_t> cg_accuracy{"cg accuracy"s, 9};
};

reference_attr_setting::reference_attr_setting()
    : p_i(std::make_unique<impl>()) {
  title_name_     = std::string{name};
  auto k_ref_view = g_reg()->view<reference_file>();
  std::transform(k_ref_view.begin(), k_ref_view.end(),
                 std::back_inserter(p_i->p_handles),
                 [](auto& in_e) {
                   return make_handle(in_e);
                 });
}

bool reference_attr_setting::get_file_info() {
  adsk::Debug::Print k_p{std::cout};
  MStatus k_status{};
  DOODLE_CHICK(k_status);
  destroy_handle(p_i->p_handles);

  MStatus k_s{};
  auto k_names = MNamespace::getNamespaces(MNamespace::rootNamespace(), false, &k_status);

  for (int l_i = 0; l_i < k_names.length(); ++l_i) {
    auto&& k_name = k_names[l_i];
    reference_file k_ref{};

    if (k_ref.set_namespace(d_str{k_name})) {
      auto k_h = make_handle();
      k_h.emplace<reference_file>(k_ref);
      p_i->p_handles.push_back(k_h);
    } else {
      DOODLE_LOG_WARN("命名空间 {} 中无有效引用", k_name);
    }
  }

  auto k_j_str = maya_file_io::get_channel_date();
  if (k_j_str.empty())
    return true;
  auto k_j = nlohmann::json::parse(k_j_str);

  for (auto& l_i : p_i->p_handles) {
    auto& k_ref = l_i.get<reference_file>();
    auto l_p    = k_ref.path;
    if (k_j.contains(l_p))
      entt_tool::load_comm<reference_file>(l_i, k_j.at(l_p));
    l_i.get<reference_file>().init_show_name();
  }
  return true;
}

void reference_attr_setting::render() {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  if (imgui::Button("保存")) {
    maya_file_io::chick_channel();
    nlohmann::json k_j{};
    for (auto& k : p_i->p_handles) {
      entt_tool::save_comm<reference_file, sim_cover_attr>(k, k_j[k.get<reference_file>().path]);
    }
    maya_file_io::replace_channel_date(k_j.dump());
  }
  dear::Child{"ref_file", ImVec2{0, viewport->WorkSize.y / 2}} && [&]() {
    if (imgui::Button("解析引用")) {
      get_file_info();
    }
    MStatus k_s{};
    MSelectionList l_select{};
    auto k_ref_view = g_reg()->view<reference_file>();

    for (auto k_e : k_ref_view) {
      auto& k_ref = k_ref_view.get<reference_file>(k_e);

      dear::TreeNode l_node{k_ref.path.c_str()};
      if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        p_i->p_current_select = make_handle(k_e);
      }
      l_node&& [&]() {
        imgui::Checkbox("解算", &(k_ref.use_sim));
        if (!k_ref.use_sim)
          return;

        if (imgui::Button("添加碰撞")) {
          k_s = MGlobal::getActiveSelectionList(l_select);
          DOODLE_CHICK(k_s);
          k_ref.set_collision_model(l_select);
        }
        ImGui::SameLine();
        if (imgui::Button("选择已添加")) {
          MGlobal::setActiveSelectionList(k_ref.get_collision_model());
        }
        dear::Text("解算碰撞: "s);
        for (const auto& k_f : k_ref.collision_model_show_str)
          dear::Text(k_f);
      };
    }
  };
  dear::Child{"sim_attr"} && [&]() {
    if (p_i->p_current_select) {
      if (p_i->p_current_select.any_of<sim_cover_attr>()) {
        if (ImGui::Checkbox(*p_i->simple_subsampling, &p_i->simple_subsampling)) {
          p_i->p_current_select.get<sim_cover_attr>()
              .simple_subsampling = p_i->simple_subsampling;
        }
        if (ImGui::InputInt(*p_i->frame_samples, &p_i->frame_samples)) {
          p_i->p_current_select.get<sim_cover_attr>()
              .frame_samples = p_i->frame_samples;
        }
        if (ImGui::InputFloat(*p_i->time_scale, &p_i->time_scale)) {
          p_i->p_current_select.get<sim_cover_attr>()
              .time_scale = p_i->time_scale;
        }
        if (ImGui::InputFloat(*p_i->length_scale, &p_i->length_scale)) {
          p_i->p_current_select.get<sim_cover_attr>()
              .length_scale = p_i->length_scale;
        }
        if (ImGui::InputInt(*p_i->max_cg_iteration, &p_i->max_cg_iteration)) {
          p_i->p_current_select.get<sim_cover_attr>()
              .max_cg_iteration = p_i->max_cg_iteration;
        }
        if (ImGui::InputInt(*p_i->cg_accuracy, &p_i->cg_accuracy)) {
          p_i->p_current_select.get<sim_cover_attr>()
              .cg_accuracy = p_i->cg_accuracy;
        }

      } else {
        if (ImGui::Button("添加配置")) {
          auto& l_value              = p_i->p_current_select.emplace<sim_cover_attr>();
          l_value.simple_subsampling = p_i->simple_subsampling;
          l_value.frame_samples      = p_i->frame_samples;
          l_value.time_scale         = p_i->time_scale;
          l_value.length_scale       = p_i->length_scale;
          l_value.max_cg_iteration   = p_i->max_cg_iteration;
          l_value.cg_accuracy        = p_i->cg_accuracy;
        }
      }
    }
  };

}

void reference_attr_setting::clear() {
  destroy_handle(p_i->p_handles);
}
reference_attr_setting::~reference_attr_setting() {
  clear();
}

}  // namespace maya_plug

}  // namespace doodle
