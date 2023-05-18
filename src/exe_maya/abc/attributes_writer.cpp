#include "attributes_writer.h"

#include "maya_plug/exception/exception.h"

#include "abc/attributes_writer.h"
#include <maya/MApiNamespace.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

namespace doodle::alembic {
bool attributes_writer::has_any_attr(const MFnDependencyNode &in_fn_transform) {
  const auto l_size = in_fn_transform.attributeCount();
  MStatus l_status{};
  for (auto i = 0; i < l_size; ++i) {
    auto l_attr = in_fn_transform.attribute(i);

    MFnAttribute l_fn_attr{l_attr, &l_status};
    maya_plug::maya_chick(l_status);

    auto l_plug = in_fn_transform.findPlug(l_attr, true);
    if (!l_fn_attr.isReadable() || l_plug.isNull()) {
      continue;
    }
  }

  return false;
}
}  // namespace doodle::alembic