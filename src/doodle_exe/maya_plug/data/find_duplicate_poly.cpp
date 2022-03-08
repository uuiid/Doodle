//
// Created by TD on 2022/3/4.
//

#include "find_duplicate_poly.h"
#include <maya/MObjectArray.h>
#include <data/maya_poly_info.h>
#include <maya/MItDependencyGraph.h>

namespace doodle::maya_plug {

std::vector<std::pair<MObject, MObject>> find_duplicate_poly::operator()(const MObjectArray& in_array) {
  std::vector<std::pair<MObject, MObject>> l_vector{};

  std::vector<MObject> l_obj{};
  for (std::uint32_t l_i = 0; l_i < in_array.length(); ++l_i) {
    l_obj.emplace_back(in_array[l_i]);
  }

  std::vector<maya_poly_info> l_multimap{};
  l_multimap = l_obj |
               ranges::views::transform([](const MObject& in_object) {
                 return maya_poly_info{in_object};
               }) |
               ranges::views::filter([](const maya_poly_info& l_info) -> bool {
                 return !l_info.maya_obj.isNull() && !l_info.is_intermediate_obj;
               }) |
               ranges::to_vector;
  auto l_v = l_multimap |
             ranges::views::group_by(std::equal_to<maya_poly_info>{}) |
             ranges::to_vector;
  //  decltype(l_v)::value_type
  l_vector = l_v |
             ranges::views::filter([](const decltype(l_v)::value_type& in_vector) -> bool {
               return in_vector.size() > 1;
             }) |
             ranges::views::filter([this](const decltype(l_v)::value_type& in_vector) -> bool {
               return ranges::any_of(in_vector, [this](const maya_poly_info& in_info) -> bool {
                        return in_info.has_skin;
                      }) &&
                      ranges::any_of(in_vector, [this](const maya_poly_info& in_info) -> bool {
                        return in_info.has_cloth;
                      });
             }) |
             ranges::views::transform([this](const decltype(l_v)::value_type& in_vector) -> std::pair<MObject, MObject> {
               auto l_m_1 = ranges::find_if(in_vector, [this](const maya_poly_info& in_info) -> bool {
                 return in_info.has_skin;
               });
               auto l_m_2 = ranges::find_if(in_vector, [this](const maya_poly_info& in_info) -> bool {
                 return !in_info.has_cloth;
               });

               return std::make_pair(l_m_1->maya_obj, l_m_2->maya_obj);
             }) |
             ranges::to_vector;

  return l_vector;
}
}  // namespace doodle::maya_plug
