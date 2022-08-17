//
// Created by TD on 2022/4/21.
//

#include "subtitle_processing.h"
#include <gui/gui_ref/ref_base.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/metadata/assets_file.h>
#include <boost/contract.hpp>

namespace doodle::gui {
class subtitle_processing::subtitle_srt_line {
 public:
  subtitle_srt_line() = default;
  explicit subtitle_srt_line(std::string in_time)
      : time_str(std::move(in_time)),
        time_begin(),
        time_end(),
        subtitle() {
    parse_time(time_str);
  };
  std::string time_str{};

  void parse_time(const std::string& in_string) {
    /// 00:00:12,000 --> 00:00:15,000
    /// 0123456789
    auto l_beg = in_string.substr(0, 12);
    auto l_end = in_string.substr(17);
    time_begin = chrono::hours{std::stoi(l_beg.substr(0, 2))} +
                 chrono::minutes{std::stoi(l_beg.substr(3, 2))} +
                 chrono::seconds{std::stoi(l_beg.substr(6, 2))} +
                 chrono::milliseconds{std::stoi(l_beg.substr(9, 3))};
    time_end = chrono::hours{std::stoi(l_end.substr(0, 2))} +
               chrono::minutes{std::stoi(l_end.substr(3, 2))} +
               chrono::seconds{std::stoi(l_end.substr(6, 2))} +
               chrono::milliseconds{std::stoi(l_end.substr(9, 3))};
    time_str = in_string;
  };

  const std::string& get_time_str() {
    time_str     = fmt::format("{:%H:%M:%S} --> {:%H:%M:%S}", time_begin, time_end);
    time_str[8]  = ',';
    time_str[25] = ',';
    return time_str;
  }

  chrono::milliseconds time_begin{};
  chrono::milliseconds time_end{};

  std::string subtitle{};
};

class subtitle_processing::impl {
 public:
  impl() = default;

  gui_cache<std::vector<std::string>> list_srt_file{"##list"s, std::vector<std::string>{}};
  gui_cache<std::string> file_suffix{"输出后缀"s, "_m"s};
  gui_cache<bool> remove_all_punctuation{"去除所有标点(不包括括号和冒号)"s, false};
  gui_cache<bool> remove_brackets_content{"去除括号内内容"s, false};
  gui_cache<bool> remove_colon_front{"去除冒号前内容"s, false};
  gui_cache<bool> cut_off_line{"切断行"s, false};
  gui_cache<std::int32_t> cut_off_line_size{"切断行大小"s, 28};

  gui_cache_name_id run_button{"开始处理"s};
};

subtitle_processing::subtitle_processing()
    : p_i(std::make_unique<impl>()) {
  title_name_ = std::string{name};
}
void subtitle_processing::init() {
  this->sig_scoped.emplace_back(
      g_reg()->ctx().at<core_sig>().select_handles.connect(
          [this](const std::vector<entt::handle>& in_vector) {
            p_i->list_srt_file =
                in_vector |
                ranges::views::filter([](const entt::handle& in_handle) -> bool {
                  return in_handle && in_handle.any_of<assets_file>();
                }) |
                ranges::views::filter([](const entt::handle& in_handle) -> bool {
                  auto& l_p = in_handle.get<assets_file>().path_attr();
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

  ImGui::InputText(*p_i->file_suffix.gui_name, &p_i->file_suffix.data);

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
                       auto l_out = in_path;
                       auto l_f   = (l_out.stem() += p_i->file_suffix.data);
                       l_f += in_path.extension();
                       l_out.remove_filename() /= l_f;
                       this->run(in_path, l_out);
                     });
  }
}
void subtitle_processing::run(const FSys::path& in_path, const FSys::path& out_subtitles_file) {
  boost::contract::check l_ =
      boost::contract::function()
          .precondition([&]() {
            DOODLE_CHICK(FSys::exists(in_path), doodle_error{"文件 {} 不存在", in_path});
            DOODLE_CHICK(in_path.extension() == ".srt", doodle_error{"文件 {} 扩展名错误", in_path});
          });

  std::vector<subtitle_srt_line> l_vector{};
  {
    FSys::ifstream l_ifstream{in_path};

    {
      /// \brief 使用循环解析字幕文件
      std::string l_string{};
      while (std::getline(l_ifstream, l_string)) {
        if (l_string.empty()) {
          continue;
        }

        auto& l_b = l_vector.emplace_back(subtitle_srt_line{});

        /// \brief 读取时间字符串
        DOODLE_CHICK(std::getline(l_ifstream, l_string), doodle_error{"文件 {} 解析错误", in_path});

        l_b.parse_time(l_string);
        /// \brief 读取字幕
        while (!l_string.empty()) {
          if (std::getline(l_ifstream, l_string)) {
            l_b.subtitle += l_string;
          } else
            break;
        }
      }
    }
  }

  for (auto& l_line : l_vector) {
    /// \brief 开始去除所有的标点符号,  不包括括号和冒号
    std::wstring l_str = conv::utf_to_utf<wchar_t>(l_line.subtitle);

    if (p_i->remove_all_punctuation.data) {
      //      static std::wregex l_wregex{LR"([\u4e00-\u9fa5]+([\(（][\u4e00-\u9fa5|\w]+[\)）])?[：:]?)"};
      static std::wregex l_wregex{LR"([^\u4e00-\u9fa5|\(|\)|（|）|：|:|\w])"};
      l_str = std::regex_replace(l_str, l_wregex, L" ");
      while (l_str.find(L"  ") != decltype(l_str)::npos) {
        boost::replace_all(l_str, L"  ", L" ");
      }
      if (!l_str.empty() && l_str.front() == L' ') {
        l_str.erase(0, 1);
      }
    }
    /// \brief 去除括号内的内容
    if (p_i->remove_brackets_content.data) {
      static std::wregex l_wregex{LR"(([\(|（].*?[\)|）]))"};
      l_str = std::regex_replace(l_str, l_wregex, L"");
    }
    /// \brief 去除冒号前的内容
    if (p_i->remove_colon_front.data) {
      static std::wregex l_wregex{LR"((.*?[：|:]))"};
      l_str = std::regex_replace(l_str, l_wregex, L"");
    }
    l_line.subtitle = conv::utf_to_utf<char>(l_str);
  }
  /// \brief 将太长的字幕打断
  if (p_i->cut_off_line.data) {
    for (auto it = l_vector.begin();
         it != l_vector.end();) {
      std::wstring l_str = conv::utf_to_utf<wchar_t>(it->subtitle);
      const auto l_size  = p_i->cut_off_line_size.data * (sizeof(decltype(it->subtitle)::value_type) / sizeof(decltype(l_str)::value_type));
      if (l_str.size() > l_size) {
        //        const auto l_size = p_i->cut_off_line_size.data;
        std::vector<subtitle_srt_line> l_sub_list{};

        /// \brief 开始分割字符串
        std::wstring l_sub{};
        std::vector<std::wstring> l_str_list{};
        boost::split(l_str_list, l_str, boost::is_any_of(L" "s));
        /// \brief 没有空格不拆分
        if (l_sub_list.size() == 1)
          break;
        l_str_list |= ranges::actions::remove_if([](auto& in) { return in.empty(); });
        for (const auto& l_item : l_str_list) {
          if (l_item.size() > l_size && l_sub.empty()) {
            /// \brief 首先查看分裂字符串 大于限制 并且没有上一个的剩余字符串后, 直接加入序列并进行 @b 下次循环
            l_sub_list.emplace_back(subtitle_srt_line{}).subtitle = conv::utf_to_utf<char>(l_item);
            continue;
          } else if ((l_sub.size() + l_item.size()) > l_size && !l_sub.empty()) {
            /// \brief 如果上一个剩余和本次字符串之和超过限制,并且剩余不空, 直接将剩余字符串添加到字幕
            l_sub_list.emplace_back(subtitle_srt_line{}).subtitle = conv::utf_to_utf<char>(l_sub);
            l_sub.clear();
          }
          /// \brief 最后将剩余字符串保存
          if (!l_sub.empty())
            l_sub += L' ';

          l_sub += l_item;
        }
        /// \brief 在最有一次循环中如果还有剩余字符串直接加入字幕
        if (!l_sub.empty())
          l_sub_list.emplace_back(subtitle_srt_line{}).subtitle = conv::utf_to_utf<char>(l_sub);

        /// \brief 设置时间
        auto l_du = (it->time_end - it->time_begin) / l_sub_list.size();
        for (auto&& [l_index, l_line] : l_sub_list | ranges::views::enumerate) {
          l_line.time_begin = it->time_begin + (l_du * l_index);
          l_line.time_end   = it->time_begin + (l_du * (l_index + 1));
        }
        it = l_vector.erase(it);
        it = l_vector.insert(it, l_sub_list.begin(), l_sub_list.end());
        it = it + l_sub_list.size();
      } else {
        ++it;
      }
    }
  }
  ranges::for_each(l_vector, [](auto& in_item) { in_item.get_time_str(); });

  std::vector<std::string> l_sub_str_list =
      l_vector |
      ranges::views::enumerate |
      ranges::views::transform(
          [&](const std::pair<std::size_t, subtitle_srt_line>& in_pair) -> std::string {
            auto l_str = fmt::format(
                R"({}
{}
{})",
                in_pair.first + 1,
                in_pair.second.time_str,
                in_pair.second.subtitle);
            return l_str;
          }) |
      ranges::to_vector;
  if (!FSys::exists(out_subtitles_file.parent_path()))
    FSys::create_directories(out_subtitles_file.parent_path());
  DOODLE_LOG_INFO("输出文件 {}", out_subtitles_file);
  FSys::ofstream{out_subtitles_file} << fmt::to_string(fmt::join(l_sub_str_list, "\n\n"));
}
subtitle_processing::~subtitle_processing() = default;
}  // namespace doodle::gui
