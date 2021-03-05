// #include

#pragma once

#if defined(MOTIONGLOBAL_LIBRARY)
#define MOTIONGLOBAL_API __declspec(dllexport)
#else
#define MOTIONGLOBAL_API __declspec(dllimport)
#endif

#include <variant>
#include <filesystem>

#define DOODLE_DISABLE_COPY(className)   \
  className(const className &) = delete; \
  className &operator=(const className &) = delete;

namespace doodle {
namespace FSys = std::filesystem;
using variant  = std::variant<
    std::string,  //
    int64_t       //
    >;
namespace motion::ui {
class TreeDirItem;
using TreeDirItemPtr = std::shared_ptr<TreeDirItem>;
};  // namespace motion::ui

}  // namespace doodle