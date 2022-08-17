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

namespace doodle::movie {

DOODLE_JSON_CPP(image_watermark,
                text_,
                width_proportion_,
                height_proportion_,
                rgba_)

DOODLE_JSON_CPP(image_attr,
                path_,
                watermarks,
                num)

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

    auto k_name = image.path_.filename().generic_string();

    auto k_b    = std::sregex_iterator{k_name.begin(), k_name.end(), reg};

    for (auto it = k_b; it != std::sregex_iterator{}; ++it) {
      k_match = *it;
      num_list.push_back(std::stoi(k_match.str()));
    }
  }
};

}  // namespace
void image_attr::extract_num(std::vector<image_attr> &in_image_list) {
  auto l_list = in_image_list |
                ranges::views::transform([](auto in_item) -> image_attr_auxiliary {
                  return image_attr_auxiliary{in_item};
                }) |
                ranges::to_vector;
  const auto k_size = l_list.front().num_list.size();

  ranges::all_of(l_list,
                 [k_size](const image_attr_auxiliary &in) -> bool {
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
  ranges::for_each(l_list,
                   [&](image_attr_auxiliary &in_attribute) {
                     in_attribute.image.num = in_attribute.num_list[l_index];
                   });
}

bool image_attr::operator<(const image_attr &in_rhs) const {
  return num < in_rhs.num;
}
bool image_attr::operator>(const image_attr &in_rhs) const {
  return in_rhs < *this;
}
bool image_attr::operator<=(const image_attr &in_rhs) const {
  return !(in_rhs < *this);
}
bool image_attr::operator>=(const image_attr &in_rhs) const {
  return !(*this < in_rhs);
}

}  // namespace doodle::movie
