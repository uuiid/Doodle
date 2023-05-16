#pragma once

#include <maya_plug/data/maya_conv_str.h>
#include <maya_plug/exception/exception.h>
#include <maya_plug/maya_plug_fwd.h>

#include "exception/exception.h"
#include <maya/MNamespace.h>
#include <maya/MStatus.h>
#include <string>
#include <vector>

namespace doodle::maya_plug::m_namespace {
inline std::string strip_namespace_from_name(const std::string& in_full_name) {
  return conv::to_s(MNamespace::stripNamespaceFromName(conv::to_ms(in_full_name)));
};
inline std::string get_namespace_from_name(const std::string& in_full_name) {
  return conv::to_s(MNamespace::getNamespaceFromName(conv::to_ms(in_full_name)));
};

inline std::vector<MObject> get_namespace_objects(const std::string& in_space_name, bool in_recursive = false) {
  MStatus l_status{};
  auto l_list = MNamespace::getNamespaceObjects(conv::to_ms(in_space_name), in_recursive, &l_status);
  maya_chick(l_status);
  std::vector<MObject> l_objects{};
  // l_objects.reserve(l_list.length());
  // l_list.get(l_objects.data());
  for (auto i = 0ul; i < l_list.length(); ++i) {
    l_objects.emplace_back(l_list[i]);
  }
  return l_objects;
}

}  // namespace doodle::maya_plug::m_namespace