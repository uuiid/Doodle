#pragma once

#include <maya/MApiNamespace.h>
namespace doodle::alembic {
class attributes_writer {
 private:
 public:
  static bool has_any_attr(const MFnDependencyNode& in_fn_transform);
};
}  // namespace doodle::alembic