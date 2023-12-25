//
// Created by TD on 2023/12/19.
//

#include "scan_assets.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/file_association.h>
#include <doodle_core/metadata/season.h>

#include <doodle_app/lib_warp/imgui_warp.h>

#include <doodle_lib/core/scan_assets/character_scan_category.h>
#include <doodle_lib/core/scan_assets/prop_scan_category.h>
#include <doodle_lib/core/scan_assets/scan_category_service.h>
#include <doodle_lib/core/scan_assets/scene_scan_category.h>

namespace doodle::gui {

namespace {
template <class Mutex>
class scan_assets_sink_t : public spdlog::sinks::base_sink<Mutex> {
  std::shared_ptr<scan_assets_t::logger_data_t> logger_data_;

 public:
  explicit scan_assets_sink_t(std::shared_ptr<scan_assets_t::logger_data_t> in_logger_data_)
      : logger_data_{std::move(in_logger_data_)} {}

 private:
 protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
    // 格式化
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    std::lock_guard const _lock{logger_data_->mutex_};
    logger_data_->data_.append(formatted.data(), formatted.size());
  }
  void flush_() override{};
};
}  // namespace

void scan_assets_t::init_scan_categories() {
  project_roots_ = {
      project_root_gui_t{"//192.168.10.250/public/DuBuXiaoYao_3", "独步逍遥", true},
      {"//192.168.10.240/public/renjianzuideyi", "人间最得意", true},
      {"//192.168.10.240/public/WuJinShenYu", "无尽神域", true},
      {"//192.168.10.240/public/WuDiJianHun", "无敌剑魂", true},
      {"//192.168.10.240/public/WanGuShenHua", "万古神话", true},
      {"//192.168.10.240/public/LianQiShiWanNian", "炼气十万年", true},
      {"//192.168.10.240/public/WGXD", "万古邪帝", true},
      {"//192.168.10.240/public/LongMaiWuShen", "龙脉武神", true},
      {"//192.168.10.218/WanYuFengShen", "万域封神", true}
  };
  scan_categories_factory_vec_ = {
      scan_categories_factory_t{
          gui_cache_name_id{"扫描角色"}, true,
          []() -> std::shared_ptr<doodle::details::scan_category_t> {
            return std::make_shared<doodle::details::character_scan_category_t>();
          }
      },
      scan_categories_factory_t{
          gui_cache_name_id{"扫描场景"}, true,
          []() -> std::shared_ptr<doodle::details::scan_category_t> {
            return std::make_shared<doodle::details::scene_scan_category_t>();
          }
      },
      scan_categories_factory_t{
          gui_cache_name_id{"扫描道具"}, true,
          []() -> std::shared_ptr<doodle::details::scan_category_t> {
            return std::make_shared<doodle::details::prop_scan_category_t>();
          }
      }
  };
  logger_data_ = std::make_shared<logger_data_t>();
  logger_      = g_logger_ctrl().make_log("scan_assets_t");
  logger_->sinks().emplace_back(std::make_shared<scan_assets_sink_t<std::mutex>>(logger_data_));
  if (!g_ctx().contains<doodle::details::scan_category_service_t>())
    g_ctx().emplace<doodle::details::scan_category_service_t>();

  create_scan_categories();
}

void scan_assets_t::create_scan_categories() {
  scan_categories_.clear();
  for (auto&& l_factory : scan_categories_factory_vec_) {
    if (!l_factory.has_) continue;
    scan_categories_.emplace_back(l_factory.factory_())->logger_ = logger_;
  }
}

void scan_assets_t::append_assets_table_data(const std::vector<doodle::details::scan_category_data_ptr>& in_data) {
  for (auto l_data : in_data) {
    if (!l_data) continue;
    scan_gui_data_t l_gui_data{};
    l_gui_data.name_          = l_data->name_;
    l_gui_data.season_        = fmt::format("季度: {}", l_data->season_.p_int);
    l_gui_data.version_name_  = l_data->version_name_;
    l_gui_data.ue_path_       = l_data->ue_file_.path_.generic_string();
    l_gui_data.maya_rig_path_ = l_data->rig_file_.path_.generic_string();
    l_gui_data.project_root_  = fmt::format("{}: {}", l_data->project_root_.name_, l_data->project_root_.path_);
    l_gui_data.info_          = "";
    assets_table_data_.emplace_back(std::move(l_gui_data));
  }
}

void scan_assets_t::start_scan() {
  scam_data_vec_.clear();
  assets_table_data_.clear();
  scan_categories_is_scan_.clear();
  for (auto&& l_root : project_roots_) {
    if (!l_root.has_) continue;
    for (auto&& l_data : scan_categories_) {
      auto l_end_ptr = scan_categories_is_scan_.emplace_back(std::make_shared<std::atomic_bool>(false));
      g_ctx().get<doodle::details::scan_category_service_t>().async_scan_files(
          l_root, l_data,
          boost::asio::bind_executor(
              g_io_context(),
              [l_end_ptr, l_root = l_root.path_,
               this](std::vector<doodle::details::scan_category_data_ptr> in_vector, boost::system::error_code in_ec) {
                *l_end_ptr = true;
                if (in_ec) {
                  logger_->log(log_loc(), level::err, "扫描资产失败:{} {}", in_ec.message(), l_root);
                  return;
                }
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
      if (ImGui::Checkbox(l_root.name_.c_str(), &l_root.has_)) {
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
      }))
    if (ImGui::Button(*start_scan_id)) {
      start_scan();
    }

  if (auto l_table = dear::Table{*assets_table_id_, boost::numeric_cast<std::int32_t>(assets_table_header_.size())};
      l_table) {
    for (auto&& l_header : assets_table_header_) {
      ImGui::TableSetupColumn(l_header.c_str());
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
        dear::Text(i.ue_path_);
        ImGui::TableNextColumn();
        dear::Text(i.maya_rig_path_);
        ImGui::TableNextColumn();
        dear::Text(i.project_root_);
        ImGui::TableNextColumn();
        dear::Text(i.info_);
      }
    }
  }

  return is_open;
}
}  // namespace doodle::gui