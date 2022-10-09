//
// Created by TD on 2022/5/16.
//

#include "move_create.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>

#include <range/v3/range.hpp>
#include <range/v3/view/all.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/algorithm/all_of.hpp>

#include <range/v3/view/iota.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/range_for.hpp>
#include <range/v3/algorithm/for_each.hpp>

#include <nlohmann/json.hpp>
#include <doodle_core/details/json_macro.h>

#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/core/core_set.h>

namespace doodle::movie {

DOODLE_JSON_CPP(image_watermark, text_attr, width_proportion_attr, height_proportion_attr, rgba_attr)
image_watermark::image_watermark(std::string in_p_text, double_t in_p_width_proportion, double_t in_p_height_proportion, image_watermark::rgba_t in_rgba)
    : text_attr(std::move(in_p_text)),
      width_proportion_attr(in_p_width_proportion),
      height_proportion_attr(in_p_height_proportion),
      rgba_attr(in_rgba) {}

DOODLE_JSON_CPP(image_attr, path_attr, watermarks_attr, num_attr)

namespace {
class image_attr_auxiliary {
 public:
  image_attr_auxiliary() = default;
  explicit image_attr_auxiliary(image_attr in_image)
      : image(std::move(in_image)) {
    extract_num_list();
  };

  std::vector<std::int32_t> num_list;
  image_attr image;
  void extract_num_list() {
    static std::regex reg{R"(\d+)"};
    std::smatch k_match{};

    auto k_name = image.path_attr.filename().generic_string();

    auto k_b    = std::sregex_iterator{k_name.begin(), k_name.end(), reg};

    for (auto it = k_b; it != std::sregex_iterator{}; ++it) {
      k_match = *it;
      num_list.push_back(std::stoi(k_match.str()));
    }
  }
};

}  // namespace

image_attr::image_attr(FSys::path in_path)
    : path_attr(std::move(in_path)) {}

void image_attr::extract_num(std::vector<image_attr> &in_image_list) {
  auto l_list = in_image_list |
                ranges::views::transform([](auto in_item) -> image_attr_auxiliary {
                  return image_attr_auxiliary{in_item};
                }) |
                ranges::to_vector;
  const auto k_size = l_list.front().num_list.size();

  ranges::all_of(l_list, [k_size](const image_attr_auxiliary &in) -> bool {
    return in.num_list.size() == k_size;
  })
      ? void()
      : throw_exception(doodle_error{"序列不匹配"s});

  DOODLE_CHICK(l_list.size() >= 2, doodle_error{"单个文件, 无法搜索帧号"});
  auto &one   = l_list[0].num_list;
  auto &tow   = l_list[1].num_list;
  auto l_item = ranges::views::ints(std::size_t{0}, k_size) |
                ranges::views::filter([&](const std::size_t &in_tuple) {
                  return one[in_tuple] != tow[in_tuple];
                }) |
                ranges::to_vector;

  DOODLE_CHICK(!l_item.empty(), doodle_error{"没有找到帧索引"});
  auto l_index = l_item.front();
  ranges::for_each(l_list, [&](image_attr_auxiliary &in_attribute) {
    in_attribute.image.num_attr = in_attribute.num_list[l_index];
  });
}

bool image_attr::operator<(const image_attr &in_rhs) const noexcept {
  return num_attr < in_rhs.num_attr;
}
bool image_attr::operator==(const image_attr &in_rhs) const noexcept {
  return path_attr == in_rhs.path_attr;
}

std::vector<image_attr> image_attr::make_default_attr(
    const entt::handle &in_handle,
    const std::vector<FSys::path> &in_path_list
) {
  std::vector<image_attr> list{};
  list = in_path_list |
         ranges::views::transform(
             [&](const FSys::path &in_path) -> image_attr {
               image_attr l_attribute{};
               l_attribute.path_attr = in_path;
               if (in_handle.any_of<shot>())
                 l_attribute.watermarks_attr.emplace_back(
                     fmt::to_string(in_handle.get<shot>()), 0.1, 0.1, image_watermark::rgb_default
                 );
               if (in_handle.any_of<episodes>())
                 l_attribute.watermarks_attr.emplace_back(
                     fmt::to_string(in_handle.get<episodes>()), 0.1, 0.15, image_watermark::rgb_default
                 );
               l_attribute.watermarks_attr.emplace_back(
                   g_reg()->ctx().at<user::current_user>().user_name_attr(), 0.1, 0.2, image_watermark::rgb_default
               );
               l_attribute.watermarks_attr.emplace_back(
                   core_set::get_set().organization_name, 0.1, 0.25, image_watermark::rgb_default
               );
               return l_attribute;
             }
         ) |
         ranges::to_vector;
  image_attr::extract_num(list);
  const auto l_size = in_path_list.size();
  ranges::for_each(list, [&](image_attr &in_attribute) {
    in_attribute.watermarks_attr.emplace_back(
        fmt::format("{}/{}", in_attribute.num_attr, l_size), 0.8, 0.1, image_watermark::rgb_default
    );
    in_attribute.watermarks_attr.emplace_back(
        fmt::format("{:%Y-%m-%d %H:%M:%S}", chrono::floor<chrono::minutes>(chrono::system_clock::now())),
        0.8, 0.2,
        image_watermark::rgb_default
    );
  });

  return list;
}

}  // namespace doodle::movie
