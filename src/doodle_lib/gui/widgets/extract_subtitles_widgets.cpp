//
// Created by TD on 2022/4/18.
//

#include "extract_subtitles_widgets.h"

#include <doodle_lib/gui/gui_ref/ref_base.h>
#include <boost/contract.hpp>

namespace doodle::gui {
class extract_subtitles_widgets::impl {
 public:
  impl() = default;
  gui_cache<std::vector<std::string>> file_list_{
      "文件列表"s, std::vector<std::string>{}};
  gui_cache_name_id export_{"导出"s};
  gui_cache<std::string> export_file_path_{"导出路径"};

  gui_cache<std::string> regex_{"正则表达式"s, R"(([^\x00-\xff]+[:|：][^\x00-\xff]+))"s};
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
  ImGui::InputText(*p_i->regex_.gui_name, &p_i->regex_.data);

  if (ImGui::Button(*p_i->export_)) {
    ranges::for_each(p_i->file_list_.data, [&](const std::string& in_string) {

    });
  };
}
void extract_subtitles_widgets::write_subtitles(const FSys::path& in_source_file,
                                                const FSys::path& out_subtitles_file) {
  boost::contract::check _l_chick =
      boost::contract::function()
          .postcondition([&]() {
            chick_true<doodle_error>(FSys::is_regular_file(in_source_file),
                                     DOODLE_LOC,
                                     "不存在文件");
            chick_true<doodle_error>(in_source_file.extension() == ".txt",
                                     DOODLE_LOC,
                                     "文件类型错误");
          });
  FSys::ifstream l_in_f{in_source_file};

  std::regex l_regex{p_i->regex_.data};
  std::vector<std::string> l_subtitles{};
  for (std::string l_string{}; std::getline(l_in_f, l_string);) {
    if (std::regex_match(l_string, l_regex))
      l_subtitles.emplace_back(l_string);
  }

  chrono::seconds l_seconds{};
  l_subtitles = l_subtitles |
                ranges::views::enumerate |
                ranges::views::transform(
                    [&](const std::pair<std::size_t, std::string>& in_pair) -> std::string {
                      auto l_str = fmt::format(
                          R"({}
{:0.2%H:0.2%M:0.2%S},000 --> {:0.2%H:0.2%M:0.2%S},000
{})",
                          in_pair.first, l_seconds, l_seconds + 3s, in_pair.second);
                      l_seconds += 3s;
                      return l_str;
                    }) |
                ranges::to_vector;

  if (!FSys::exists(out_subtitles_file.parent_path()))
    FSys::create_directories(out_subtitles_file.parent_path());
  FSys::ofstream{out_subtitles_file} << fmt::to_string(fmt::join(l_subtitles, "\n"));
}
extract_subtitles_widgets::~extract_subtitles_widgets() = default;
}  // namespace doodle::gui
