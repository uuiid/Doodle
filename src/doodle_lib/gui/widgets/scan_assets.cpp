//
// Created by TD on 2023/12/19.
//

#include "scan_assets.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_app/lib_warp/imgui_warp.h>

#include <doodle_lib/core/scan_assets/character_scan_category.h>
#include <doodle_lib/core/scan_assets/prop_scan_category.h>
#include <doodle_lib/core/scan_assets/scan_category_service.h>
#include <doodle_lib/core/scan_assets/scene_scan_category.h>

namespace doodle::gui {
// namespace {
template <class Mutex>
class scan_sink_t : public spdlog::sinks::base_sink<Mutex> {
  std::shared_ptr<scan_assets_t::logger_data_t> logger_data_;

 public:
  explicit scan_sink_t(std::shared_ptr<scan_assets_t::logger_data_t> in_logger_data_)
      : logger_data_{std::move(in_logger_data_)} {}

 private:
 protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
    // 格式化
    std::lock_guard const _lock{logger_data_->mutex_};
    logger_data_->data_.append(msg.payload.data(), msg.payload.size());
    logger_data_->data_.append("\n");
  }
  void flush_() override{};
};
// }  // namespace
using scan_assets_scan_sink_t = scan_sink_t<std::mutex>;
void scan_assets_t::init_scan_categories() {
  logger_data_ = std::make_shared<logger_data_t>();
  auto l_list    = register_file_type::get_project_list();
  project_roots_ = l_list | ranges::views::transform([](const project& in_project) -> project_root_gui_t {
                     project_root_gui_t l_root{};
                     l_root.has_       = false;
                     l_root.p_path     = std::move(in_project.p_path);
                     l_root.p_name     = std::move(in_project.p_name);
                     l_root.p_en_str   = std::move(in_project.p_en_str);
                     l_root.p_shor_str = std::move(in_project.p_shor_str);
                     return l_root;
                   }) |
                   ranges::to_vector;
  scan_categories_factory_vec_ = {
      scan_categories_factory_t{
          gui_cache_name_id{"扫描角色"}, false,
          []() -> std::shared_ptr<doodle::details::scan_category_t> {
            return std::make_shared<doodle::details::character_scan_category_t>();
          }
      },
      scan_categories_factory_t{
          gui_cache_name_id{"扫描场景"}, false,
          []() -> std::shared_ptr<doodle::details::scan_category_t> {
            return std::make_shared<doodle::details::scene_scan_category_t>();
          }
      },
      scan_categories_factory_t{
          gui_cache_name_id{"扫描道具"}, false,
          []() -> std::shared_ptr<doodle::details::scan_category_t> {
            return std::make_shared<doodle::details::prop_scan_category_t>();
          }
      }
  };

  if (!g_ctx().contains<doodle::details::scan_category_service_t>()) {
    g_ctx().emplace<doodle::details::scan_category_service_t>().logger_->sinks().emplace_back(
        std::make_shared<scan_assets_scan_sink_t>(logger_data_)
    );
  }

  create_scan_categories();
}

void scan_assets_t::create_scan_categories() {
  scan_categories_.clear();
  for (auto&& l_factory : scan_categories_factory_vec_) {
    if (!l_factory.has_) continue;
    scan_categories_.emplace_back(l_factory.factory_());
  }
}

void scan_assets_t::append_assets_table_data(const std::vector<doodle::details::scan_category_data_ptr>& in_data) {
  auto l_list = in_data;

  std::ranges::sort(l_list, [](const auto& in_l, const auto& in_r) -> bool { return in_l->name_ < in_r->name_; });
  // 检查无基础版本的情况
  std::string l_name_str{};
  for (auto l_it = l_list.begin(); l_it != l_list.end();) {
    l_name_str  = (*l_it)->name_;
    auto l_next = l_it;
    ++l_next;

    if (l_next == l_list.end()) break;

    if ((*l_next)->name_ == l_name_str && (*l_it)->ue_file_.path_.empty() && (*l_it)->rig_file_.path_.empty() &&
        (*l_it)->version_name_.empty()) {
      default_logger_raw()->log(log_loc(), level::info, "删除无效数据:{}, 版本, UE, Rig都是空", (*l_it)->base_path_);
      l_it   = l_list.erase(l_it);
      l_next = l_it;
      ++l_next;
    }
    while (l_next != l_list.end()) {
      if ((*l_next)->name_ != l_name_str) break;
      if ((*l_next)->ue_file_.path_.empty() && (*l_next)->rig_file_.path_.empty() && (*l_next)->version_name_.empty()) {
        l_next = l_list.erase(l_next);
        default_logger_raw()->log(
            log_loc(), level::info, "删除无效数据:{}, 版本, UE, Rig都是空", (*l_next)->base_path_
        );
        break;
      }
      ++l_next;
    }
    l_it = l_next;
  }

  for (auto l_data : l_list) {
    if (!l_data) continue;
    scan_gui_data_t l_gui_data{};
    l_gui_data.name_         = l_data->name_;
    l_gui_data.season_       = fmt::format("季度: {}", l_data->season_.p_int);
    l_gui_data.version_name_ = l_data->version_name_;
    l_gui_data.base_path_    = l_data->base_path_;
    l_gui_data.base_path_show =
        gui_cache_name_id{l_data->base_path_.lexically_proximate(l_data->project_root_.p_path).generic_string()};
    if (l_data->ue_file_.path_.empty()) {
      l_gui_data.ue_path_       = "未找到路径";
      l_gui_data.ue_path_color_ = ImVec4{1.0f, 0.0f, 0.0f, 1.0f};
    } else {
      l_gui_data.ue_path_ = l_data->ue_file_.path_.lexically_proximate(l_data->project_root_.p_path).generic_string();
    }
    if (l_data->rig_file_.path_.empty()) {
      l_gui_data.maya_rig_path_       = "未找到路径";
      l_gui_data.maya_rig_path_color_ = ImVec4{1.0f, 0.0f, 0.0f, 1.0f};
    } else {
      l_gui_data.maya_rig_path_ =
          l_data->rig_file_.path_.lexically_proximate(l_data->project_root_.p_path).generic_string();
    }

    l_gui_data.project_root_ = l_data->project_root_.p_path.generic_string();
    l_gui_data.info_         = fmt::format("{}/{}", l_data->project_root_.p_name, l_data->file_type_);
    assets_table_data_.emplace_back(std::move(l_gui_data));
  }
  assets_table_data_ |=
      ranges::actions::sort([](const auto& in_l, const auto& in_r) -> bool { return in_l.name_ < in_r.name_; });
}

void scan_assets_t::start_scan() {
  scam_data_vec_.clear();
  assets_table_data_.clear();
  scan_categories_is_scan_.clear();
  logger_data_->clear();
  for (auto&& l_root : project_roots_) {
    if (!l_root.has_) continue;
    for (auto&& l_data : scan_categories_) {
      auto l_end_ptr = scan_categories_is_scan_.emplace_back(std::make_shared<std::atomic_bool>(false));
      g_ctx().get<doodle::details::scan_category_service_t>().async_scan_files(
          l_root, l_data,
          boost::asio::bind_executor(
              g_io_context(),
              [l_end_ptr, l_root = l_root.p_path,
               this](std::vector<doodle::details::scan_category_data_ptr> in_vector, boost::system::error_code in_ec) {
                *l_end_ptr = true;
                if (in_ec) {
                  default_logger_raw()->log(log_loc(), level::err, "扫描资产失败:{} {}", in_ec.message(), l_root);
                  return;
                }
                default_logger_raw()->log(
                    log_loc(), level::info, "扫描资产完成:{}, 共 {} 条", l_root, in_vector.size()
                );
                append_assets_table_data(in_vector);
              }
          )
      );
    }
  }
}

bool scan_assets_t::render() {
  bool l_changed{};

  if (auto l_table = dear::Table{"项目列表", 3}; l_table) {
    for (auto&& l_root : project_roots_) {
      ImGui::TableNextColumn();
      if (ImGui::Checkbox(l_root.p_name.c_str(), &l_root.has_)) {
        l_changed = true;
      }
    }
  }

  for (auto&& l_factory : scan_categories_factory_vec_) {
    if (ImGui::Checkbox(*l_factory.name_id_, &l_factory.has_)) {
      l_changed = true;
    }
    ImGui::SameLine();
  }
  ImGui::NewLine();
  if (l_changed) {
    create_scan_categories();
  }
  if (scan_categories_is_scan_.empty() ||
      std::all_of(scan_categories_is_scan_.begin(), scan_categories_is_scan_.end(), [](auto&& i) -> bool {
        return *i;
      })) {
    if (ImGui::Button(*start_scan_id)) {
      start_scan();
    }
  } else {
    ImGui::Text("正在扫描中...");
  }

  if (auto l_child = dear::Child{"数据列表", ImVec2{0, -60}}; l_child) {
    if (auto l_table =
            dear::Table{
                *assets_table_id_, boost::numeric_cast<std::int32_t>(assets_table_header_.size()),
                ImGuiTableFlags_::ImGuiTableFlags_ScrollY | ImGuiTableFlags_::ImGuiTableFlags_ScrollX |
                    ImGuiTableFlags_::ImGuiTableFlags_RowBg | ImGuiTableFlags_::ImGuiTableFlags_BordersOuter |
                    ImGuiTableFlags_::ImGuiTableFlags_BordersV | ImGuiTableFlags_::ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_::ImGuiTableFlags_Reorderable | ImGuiTableFlags_::ImGuiTableFlags_Hideable |
                    ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit
            };
        l_table) {
      for (auto&& [l_header, l_] : assets_table_header_) {
        ImGui::TableSetupColumn(l_header.c_str(), ImGuiTableColumnFlags_None, l_);
      }
      ImGui::TableHeadersRow();

      ImGuiListClipper clipper{};
      clipper.Begin(boost::numeric_cast<std::int32_t>(assets_table_data_.size()));
      while (clipper.Step()) {
        for (auto l_i = clipper.DisplayStart; l_i < clipper.DisplayEnd; ++l_i) {
          std::size_t l_index{boost::numeric_cast<std::size_t>(l_i)};
          auto&& i = assets_table_data_[l_index];
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          dear::Text(i.name_);
          ImGui::TableNextColumn();
          dear::Text(i.season_);
          ImGui::TableNextColumn();
          dear::Text(i.version_name_);

          ImGui::TableNextColumn();
          if (dear::Selectable(*i.base_path_show)) FSys::open_explorer(i.base_path_);

          ImGui::TableNextColumn();
          if (i.ue_path_color_.x == 0.0f && i.ue_path_color_.y == 0.0f && i.ue_path_color_.z == 0.0f &&
              i.ue_path_color_.w == 0.0f) {
            dear::Text(i.ue_path_);
          } else {
            if (dear::WithStyleColor l_color{ImGuiCol_Text, i.ue_path_color_}) dear::Text(i.ue_path_);
          }
          ImGui::TableNextColumn();
          if (i.maya_rig_path_color_.x == 0.0f && i.maya_rig_path_color_.y == 0.0f &&
              i.maya_rig_path_color_.z == 0.0f && i.maya_rig_path_color_.w == 0.0f) {
            dear::Text(i.maya_rig_path_);
          } else {
            if (dear::WithStyleColor l_color{ImGuiCol_Text, i.maya_rig_path_color_}) dear::Text(i.maya_rig_path_);
          }

          ImGui::TableNextColumn();
          dear::Text(i.project_root_);
          ImGui::TableNextColumn();
          dear::Text(i.info_);
        }
      }
    }
  }

  if (auto l_child = dear::Child{"日志", ImVec2{0, 60}}; l_child) {
    {
      std::lock_guard l_lock{logger_data_->mutex_};
      log_str_ = logger_data_->data_;
    }

    ImGui::TextUnformatted(log_str_.c_str());
  }

  return is_open;
}
}  // namespace doodle::gui