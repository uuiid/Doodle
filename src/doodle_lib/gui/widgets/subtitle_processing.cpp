//
// Created by TD on 2022/4/21.
//

#include "subtitle_processing.h"
#include <gui/gui_ref/ref_base.h>
#include <core/core_sig.h>
#include <metadata/assets_file.h>
#include <boost/contract.hpp>

namespace doodle::gui {
class subtitle_processing::subtitle_srt_line {
 public:
  subtitle_srt_line() = default;
  subtitle_srt_line(std::string in_time)
      : time_str(std::move(in_time)),
        time_begin(),
        time_end(),
        subtitle() {
    parse_time();
  };
  std::string time_str{};

  void parse_time() { parse_time(time_str); };
  void parse_time(const std::string& in_string) {
    /// 00:00:12,000 --> 00:00:15,000
    /// 0123456789
    auto l_beg = in_string.substr(0, 12);
    auto l_end = in_string.substr(16);
    time_begin = chrono::hours{std::stoi(l_beg.substr(0, 2))} +
                 chrono::minutes{std::stoi(l_beg.substr(3, 2))} +
                 chrono::seconds{std::stoi(l_beg.substr(6, 2))} +
                 chrono::milliseconds{std::stoi(l_beg.substr(9, 3))};
    time_end = chrono::hours{std::stoi(l_end.substr(0, 2))} +
               chrono::minutes{std::stoi(l_end.substr(3, 2))} +
               chrono::seconds{std::stoi(l_end.substr(6, 2))} +
               chrono::milliseconds{std::stoi(l_end.substr(9, 3))};
  };

  chrono::milliseconds time_begin{};
  chrono::milliseconds time_end{};

  std::string subtitle{};
};

class subtitle_processing::impl {
 public:
  impl() = default;

  gui_cache<std::vector<std::string>> list_srt_file{"##list"s, std::vector<std::string>{}};
  gui_cache<bool> remove_all_punctuation{"去除所有标点(不包括括号和冒号)"s, false};
  gui_cache<bool> remove_brackets_content{"去除括号内内容"s, false};
  gui_cache<bool> remove_colon_front{"去除冒号前内容"s, false};
  gui_cache<bool> cut_off_line{"切断行"s, false};
  gui_cache<std::int32_t> cut_off_line_size{"切断行大小"s, 14};

  gui_cache_name_id run_button{"开始处理"s};
};

subtitle_processing::subtitle_processing()
    : p_i(std::make_unique<impl>()) {
}
void subtitle_processing::init() {
  window_panel::init();

  this->sig_scoped.emplace_back(
      g_reg()->ctx().at<core_sig>().select_handles.connect(
          [this](const std::vector<entt::handle>& in_vector) {
            p_i->list_srt_file =
                in_vector |
                ranges::views::filter([](const entt::handle& in_handle) -> bool {
                  return in_handle && in_handle.any_of<assets_file>();
                }) |
                ranges::views::filter([](const entt::handle& in_handle) -> bool {
                  auto& l_p = in_handle.get<assets_file>().path;
                  return l_p.extension() == ".srt";
                }) |
                ranges::views::transform([](const entt::handle& in_handle) -> std::string {
                  return in_handle.get<assets_file>().get_path_normal().generic_string();
                }) |
                ranges::to_vector;
          }));
}
void subtitle_processing::render() {
  dear::ListBox{*p_i->list_srt_file.gui_name} && [&]() {
    for (auto&& i : p_i->list_srt_file.data) {
      dear::Text(i);
    }
  };

  ImGui::Checkbox(*p_i->remove_all_punctuation.gui_name, &p_i->remove_all_punctuation.data);
  ImGui::Checkbox(*p_i->remove_brackets_content.gui_name, &p_i->remove_brackets_content.data);
  ImGui::Checkbox(*p_i->remove_colon_front.gui_name, &p_i->remove_colon_front.data);
  ImGui::Checkbox(*p_i->cut_off_line.gui_name, &p_i->cut_off_line.data);
  if (p_i->cut_off_line.data) {
    ImGui::InputInt(*p_i->cut_off_line_size.gui_name, &p_i->cut_off_line_size.data);
  }
  if (ImGui::Button(*p_i->run_button)) {
    ranges::for_each(p_i->list_srt_file.data,
                     [this](const FSys::path& in_path) {
                       this->run(in_path);
                     });
  }
}
void subtitle_processing::run(const FSys::path& in_path) {
  boost::contract::check l_ =
      boost::contract::function()
          .precondition([&]() {
            chick_true<doodle_error>(FSys::exists(in_path), DOODLE_LOC, "文件 {} 不存在", in_path);
            chick_true<doodle_error>(in_path.extension() == ".srt", DOODLE_LOC, "文件 {} 扩展名错误", in_path);
          });

  FSys::ifstream l_ifstream{in_path};

  std::vector<subtitle_srt_line> l_vector{};
  {
    /// \brief 使用循环解析字幕文件
    std::string l_string{};
    while (std::getline(l_ifstream, l_string)) {
      auto& l_b = l_vector.emplace_back(subtitle_srt_line{});

      /// \brief 读取时间字符串
      chick_true<doodle_error>(std::getline(l_ifstream, l_string), DOODLE_LOC, "文件 {} 解析错误", in_path);

      l_b.time_str = l_string;
      /// \brief 读取字幕
      while (!l_string.empty()) {
        chick_true<doodle_error>(std::getline(l_ifstream, l_string), DOODLE_LOC, "文件 {} 解析错误", in_path);
        l_b.subtitle += l_string;
      }
    }
  }

  for (auto l_line : l_vector) {
    /// \brief 开始去除所有的标点符号,  不包括括号和冒号
    std::wstring l_str = conv::utf_to_utf<wchar_t>(l_line.subtitle);

    if (p_i->remove_all_punctuation.data) {
      static std::wregex l_wregex{LR"([^\u4e00-\u9fa5|\(|\)|（|）|：|:])"};
      l_str = std::regex_replace(l_str, l_wregex, L" ");
    }
    /// \brief 去除括号内的内容
    if (p_i->remove_brackets_content.data) {
      static std::wregex l_wregex{LR"(([\(|（].*?[\)|）]))"};
      l_str = std::regex_replace(l_str, l_wregex, L" ");
    }
    /// \brief 去除冒号前的内容
    if (p_i->remove_colon_front.data) {
      static std::wregex l_wregex{LR"((.*?[：|:]))"};
      l_str = std::regex_replace(l_str, l_wregex, L" ");
    }
    l_line.subtitle = conv::utf_to_utf<char>(l_str);
  }
  /// \brief 将太长的字幕打断
  if (p_i->cut_off_line.data) {
    for (auto it = l_vector.begin();
         it != l_vector.end();) {
      if (it->subtitle.size() > p_i->cut_off_line_size.data) {
        //        std::wstring l_str = conv::utf_to_utf<wchar_t>(it->subtitle);
        //        const auto l_size  = p_i->cut_off_line_size.data *
        //                            sizeof(decltype(l_str)::value_type) /
        //                            sizeof(decltype(it->subtitle)::value_type);
        const auto l_size = p_i->cut_off_line_size.data;
        std::vector<subtitle_srt_line> l_sub_list{};

        /// \brief 开始分割字符串
        std::string l_sub{};
        std::vector<std::string> l_str_list{};
        boost::split(l_str_list, it->subtitle, boost::is_any_of(" "));
        for (auto l_item : l_str_list) {
          if ((l_sub.size() + l_item.size()) > l_size) {
            l_sub_list.emplace_back(subtitle_srt_line{}).subtitle = std::move(l_sub);
          }
          l_sub += l_item;
        }

      }
    }
  }
  //  if (p_i->cut_off_line.data) {
  //    auto l_cut_ = p_i->cut_off_line_size.data;
  //    for (
  //        auto l_it = l_str.find_last_of(L' ', l_cut_);
  //        l_it != decltype(l_str)::npos;
  //        l_it = l_str.find_last_of(L' ', l_cut_ + l_it)) {
  //      l_str[l_it] = L'\n';
  //    }
  //  }
}
subtitle_processing::~subtitle_processing() = default;
}  // namespace doodle::gui
