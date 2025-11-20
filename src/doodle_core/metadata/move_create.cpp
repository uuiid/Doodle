//
// Created by TD on 2022/5/16.
//

#include "move_create.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/user.h>

#include <fmt/chrono.h>
#include <nlohmann/json.hpp>
#include <range/v3/algorithm/all_of.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/range.hpp>
#include <range/v3/range_for.hpp>
#include <range/v3/view/all.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/transform.hpp>

namespace doodle::movie {

image_watermark::image_watermark(
    std::string in_p_text, double_t in_p_width_proportion, double_t in_p_height_proportion,
    image_watermark::rgba_t in_rgba
)
    : text_attr(std::move(in_p_text)),
      width_proportion_attr(in_p_width_proportion),
      height_proportion_attr(in_p_height_proportion),
      rgba_attr(in_rgba) {}

namespace {
class image_attr_auxiliary {
 public:
  explicit image_attr_auxiliary(image_attr& in_image) : image(&in_image) { extract_num_list(); };

  std::vector<std::int32_t> num_list;
  image_attr* image;

  void extract_num_list() {
    static std::regex reg{R"(\d+)"};
    std::smatch k_match{};

    auto k_name = image->path_attr.filename().generic_string();

    auto k_b    = std::sregex_iterator{k_name.begin(), k_name.end(), reg};

    for (auto it = k_b; it != std::sregex_iterator{}; ++it) {
      k_match = *it;
      num_list.push_back(std::stoi(k_match.str()));
    }
  }
};
}  // namespace

image_attr::image_attr(FSys::path in_path) : path_attr(std::move(in_path)) {}

void image_attr::extract_num(std::vector<image_attr>& in_image_list) {
  auto l_list =
      in_image_list |
      ranges::views::transform([](auto&& in_item) -> image_attr_auxiliary { return image_attr_auxiliary{in_item}; }) |
      ranges::to_vector;
  const auto k_size = l_list.front().num_list.size();

  ranges::all_of(l_list, [k_size](const image_attr_auxiliary& in) -> bool { return in.num_list.size() == k_size; })
      ? void()
      : throw_exception(doodle_error{"序列不匹配"s});

  if (l_list.size() < 2) {
    default_logger_raw()->log(log_loc(), level::warn, "传入的序列只有一个，无法进行比较");
    return;
  }

  auto& one   = l_list[0].num_list;
  auto& tow   = l_list[1].num_list;
  auto l_item = ranges::views::ints(std::size_t{0}, k_size) |
                ranges::views::filter([&](const std::size_t& in_tuple) { return one[in_tuple] != tow[in_tuple]; }) |
                ranges::to_vector;

  if (l_item.empty()) {
    default_logger_raw()->log(log_loc(), level::warn, "没有找到序列号");
    return;
  }
  auto l_index = l_item.front();
  ranges::for_each(l_list, [&](image_attr_auxiliary& in_attribute) {
    in_attribute.image->num_attr = in_attribute.num_list[l_index];
  });
}

bool image_attr::operator<(const image_attr& in_rhs) const noexcept { return num_attr < in_rhs.num_attr; }
bool image_attr::operator==(const image_attr& in_rhs) const noexcept { return path_attr == in_rhs.path_attr; }

std::vector<image_attr> image_attr::make_default_attr(
    const episodes* in_episodes, const shot* in_shot, const std::vector<FSys::path>& in_path_list
) {
  std::vector<image_attr> list{};
  list =
      in_path_list | ranges::views::transform([&](const FSys::path& in_path) -> image_attr {
        image_attr l_attribute{};
        l_attribute.path_attr = in_path;
        if (in_episodes)
          l_attribute.watermarks_attr.emplace_back(
              fmt::format("ep{:04}", in_episodes->p_episodes), 0.08, 0.1, image_watermark::rgb_default
          );

        if (in_shot)
          l_attribute.watermarks_attr.emplace_back(
              fmt::format("sc{:04}{}", in_shot->p_shot, in_shot->p_shot_enum), 0.15, 0.1, image_watermark::rgb_default
          );

        return l_attribute;
      }) |
      ranges::to_vector;
  image_attr::extract_num(list);
  const auto l_size = in_path_list.size();
  ranges::for_each(list, [&](image_attr& in_attribute) {
    in_attribute.watermarks_attr.emplace_back(
        fmt::format("{}/{}", in_attribute.num_attr, l_size), 0.7, 0.1, image_watermark::rgb_default
    );

    in_attribute.watermarks_attr.emplace_back(
        fmt::format(
            "{:%Y-%m-%d %H:%M:%S}",
            chrono::floor<chrono::seconds>(chrono::current_zone()->to_local(chrono::system_clock::now()))
        ),
        0.8, 0.1, image_watermark::rgb_default
    );
  });

  return list;
}
std::vector<image_attr> image_attr::make_default_attr(const std::vector<FSys::path>& in_path_list) {
  std::vector<image_attr> list{};
  list = in_path_list | ranges::views::transform([&](const FSys::path& in_path) -> image_attr {
           image_attr l_attribute{};
           l_attribute.path_attr = in_path;
           return l_attribute;
         }) |
         ranges::to_vector;
  image_attr::extract_num(list);
  return list;
}
}  // namespace doodle::movie