//
// Created by TD on 2022/4/18.
//

#include "extract_subtitles_widgets.h"

#include <doodle_lib/gui/gui_ref/ref_base.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/core/core_sig.h>
#include <boost/contract.hpp>

namespace doodle::gui {
class extract_subtitles_widgets::impl {
 public:
  impl() = default;
  gui_cache<std::vector<std::string>> file_list_{
      "文件列表"s, std::vector<std::string>{}};
  gui_cache_name_id export_{"导出"s};
  gui_cache<std::string> export_file_path_{"导出路径", ""s};

  gui_cache<std::string> regex_find_subtitles{"正则表达式"s, R"((.+?[:|：].+?)$)"s};
  gui_cache<std::string> regex1{"正则表达式"s, R"(.+?[:|：](.+?))"s};
};

extract_subtitles_widgets::extract_subtitles_widgets()
    : p_i(std::make_unique<impl>()) {
  title_name_ = std::string{name};
}
void extract_subtitles_widgets::render() {
  dear::ListBox{*p_i->file_list_.gui_name} && [&]() {
    for (auto&& i : p_i->file_list_.data) {
      dear::Text(i);
    }
  };
  ImGui::InputText(*p_i->export_file_path_.gui_name, &p_i->export_file_path_.data);
  ImGui::InputText(*p_i->regex_find_subtitles.gui_name, &p_i->regex_find_subtitles.data);

  if (ImGui::Button(*p_i->export_)) {
    ranges::for_each(p_i->file_list_.data, [&](const FSys::path& in_string) {
      FSys::path l_out{p_i->export_file_path_.data / in_string.filename()};
      if (p_i->export_file_path_.data.empty())
        l_out = in_string;
      l_out.replace_extension(".srt");
      write_subtitles(in_string, l_out);
    });
  };
}
void extract_subtitles_widgets::write_subtitles(const FSys::path& in_source_file,
                                                const FSys::path& out_subtitles_file) {
  boost::contract::check _l_chick =
      boost::contract::function()
          .postcondition([&]() {
            FSys::is_regular_file(in_source_file)
                ? void()
                : throw_exception(doodle_error{"不存在文件 {}"s, in_source_file});
            (in_source_file.extension() == ".txt")
                ? void()
                : throw_exception(doodle_error{"文件类型错误 {} 不是txt"s, in_source_file});
          });
  FSys::ifstream l_in_f{in_source_file};

  std::wregex l_regex{conv::utf_to_utf<wchar_t>(p_i->regex_find_subtitles.data)};
  std::vector<std::string> l_subtitles{};
  for (std::string l_string{}; std::getline(l_in_f, l_string);) {
    if (std::regex_match(conv::utf_to_utf<wchar_t>(l_string), l_regex))
      l_subtitles.push_back(l_string);
  }

  chrono::seconds l_seconds{0h + 0min + 0s};
  l_subtitles = l_subtitles |
                ranges::views::enumerate |
                ranges::views::transform(
                    [&](const std::pair<std::size_t, std::string>& in_pair) -> std::string {
                      auto l_str = fmt::format(
                          R"({}
{:%H:%M:%S},000 --> {:%H:%M:%S},000
{})",
                          in_pair.first + 1, l_seconds, l_seconds + 3s, in_pair.second);
                      l_seconds += 3s;
                      return l_str;
                    }) |
                ranges::to_vector;

  if (!FSys::exists(out_subtitles_file.parent_path()))
    FSys::create_directories(out_subtitles_file.parent_path());
  DOODLE_LOG_INFO("输出文件 {}", out_subtitles_file);
  FSys::ofstream{out_subtitles_file} << fmt::to_string(fmt::join(l_subtitles, "\n\n"));
}
void extract_subtitles_widgets::init() {
  g_reg()->ctx().at<core_sig>().select_handles.connect([this](const std::vector<entt::handle>& in_list) {
    p_i->file_list_.data =
        in_list |
        ranges::views::filter(
            [](const entt::handle& in_handle) {
              return in_handle.any_of<assets_file>() &&
                     in_handle.get<assets_file>().path_attr().extension() == ".txt" &&
                     FSys::exists(in_handle.get<assets_file>().get_path_normal());
            }) |
        ranges::views::transform([](const entt::handle& in_handle) -> std::string {
          return in_handle.get<assets_file>().get_path_normal().generic_string();
        }) |
        ranges::to_vector;
  });
}
extract_subtitles_widgets::~extract_subtitles_widgets() = default;
}  // namespace doodle::gui
