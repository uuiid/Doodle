//
// Created by TD on 2021/6/17.
//

#pragma once

#define DOODLE_USE_MOVE(class_name)                          \
  class_name(class_name &&in) noexcept            = default; \
  class_name &operator=(class_name &&in) noexcept = default;

#define DOODLE_DIS_COPY(class_name)                              \
  class_name(const class_name &in) noexcept            = delete; \
  class_name &operator=(const class_name &in) noexcept = delete;

#define DOODLE_MOVE(class_name) \
  DOODLE_USE_MOVE(class_name)   \
  DOODLE_DIS_COPY(class_name)

#define DOODLE_IMP_MOVE(class_name)              \
  class_name(class_name &&) noexcept;            \
  class_name &operator=(class_name &&) noexcept; \
  DOODLE_DIS_COPY(class_name)

#define DOODLE_MOVE_CPP(class_name)                                    \
  class_name::class_name(class_name &&) noexcept            = default; \
  class_name &class_name::operator=(class_name &&) noexcept = default;

#define DOODLE_JSON(class_name)                                                            \
  friend void to_json(nlohmann::json &nlohmann_json_j, const class_name &nlohmann_json_t); \
  friend void from_json(const nlohmann::json &nlohmann_json_j, class_name &nlohmann_json_t);
