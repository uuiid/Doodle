//
// Created by TD on 2022/5/16.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <rttr/rttr_enable.h>

namespace doodle::movie {
class DOODLE_CORE_API image_watermark {
  RTTR_ENABLE();

 private:
  friend void DOODLE_CORE_API to_json(nlohmann::json& nlohmann_json_j, const image_watermark& nlohmann_json_t);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& nlohmann_json_j, image_watermark& nlohmann_json_t);

 public:
  using rgba_t      = std::array<std::double_t, 4>;
  image_watermark() = default;
  image_watermark(
      std::string in_p_text, double_t in_p_width_proportion, double_t in_p_height_proportion, rgba_t in_rgba
  );
  std::string text_attr{};
  std::double_t width_proportion_attr{};
  std::double_t height_proportion_attr{};
  rgba_t rgba_attr{};
  constexpr static const rgba_t rgb_default{25, 220, 2};
};

class DOODLE_CORE_API image_attr : boost::totally_ordered<image_attr> {
  RTTR_ENABLE();

 private:
  friend void DOODLE_CORE_API to_json(nlohmann::json& nlohmann_json_j, const image_attr& nlohmann_json_t);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& nlohmann_json_j, image_attr& nlohmann_json_t);

 public:
  image_attr() = default;
  explicit image_attr(FSys::path in_path);

  FSys::path path_attr{};
  std::vector<image_watermark> watermarks_attr{};
  std::int32_t num_attr{};

  static void extract_num(std::vector<image_attr>& in_image_list);

  static std::vector<image_attr> make_default_attr(
      const entt::handle& in_handle, const std::vector<FSys::path>& in_path_list
  );

  bool operator<(const image_attr& in_rhs) const noexcept;
  bool operator==(const image_attr& in_rhs) const noexcept;
};

}  // namespace doodle::movie
