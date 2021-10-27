//
// Created by TD on 2021/6/17.
//

#pragma once

#define DOODLE_USE_MOVE(class_name)             \
  class_name(class_name &&) noexcept = default; \
  class_name &operator=(class_name &&) = default;

#define DOODLE_DIS_COPY(class_name)           \
  class_name(class_name &) noexcept = delete; \
  class_name &operator=(class_name &) = delete;

#define DOODLE_MOVE(class_name) \
  DOODLE_USE_MOVE(class_name)   \
  DOODLE_DIS_COPY(class_name)