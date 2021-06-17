//
// Created by TD on 2021/6/17.
//

#pragma once

#define DOODLE_DISABLE_COPY(className)   \
  className(const className &) = delete; \
  className &operator=(const className &) = delete;

#define DOODLE_UE_PATH "Engine/Binaries/Win64/UE4Editor.exe"

#define DOODLE_STATIC(getValueName, type_, value) \
  namespace staticValue {                         \
  type_ getValueName() {                          \
    const static type_ install{value};            \
    return install;                               \
  };                                              \
  };
#define DOODLE_STR_STATIC(getValueName, value) \
  DOODLE_STATIC(getValueName, std::string, #value)
#define DOODLE_STR_S(value) \
  DOODLE_STR_STATIC(value##Obj, value)

#define DOODLE_STATIC_DECLARE(getValueName, type_, value) \
  namespace staticValue {                                 \
  type_ getValueName();                                   \
  };

#define DOODLE_STR_STATIC_DECLARE(getValueName, value) \
  DOODLE_STATIC_DECLARE(getValueName, std::string, #value)
#define DOODLE_STR_S_DECLARE(value) \
  DOODLE_STR_STATIC_DECLARE(value##Obj, value)
