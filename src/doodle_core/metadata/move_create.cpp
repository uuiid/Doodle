//
// Created by TD on 2022/5/16.
//

#include "move_create.h"

#include <range/v3/range.hpp>
#include <range/v3/view/all.hpp>
#include <range/v3/view/transform.hpp>
namespace doodle::move {

namespace {
class image_attr_auxiliary {
 public:
  image_attr_auxiliary() = default;
  explicit image_attr_auxiliary(image_attr in_image) : image(std::move(in_image)){};

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

}  // namespace doodle::move
