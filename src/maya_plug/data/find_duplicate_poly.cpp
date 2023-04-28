//
// Created by TD on 2022/3/4.
//

#include "find_duplicate_poly.h"

#include "maya/MObject.h"
#include "range/v3/algorithm/find_if.hpp"
#include <data/maya_poly_info.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MObjectArray.h>
#include <utility>

namespace doodle::maya_plug {

std::vector<std::pair<MObject, MObject>> find_duplicate_poly::operator()(const MObjectArray& in_array) {
  std::vector<std::pair<MObject, MObject>> l_vector{};

  std::vector<MObject> l_obj{};
  for (std::uint32_t l_i = 0; l_i < in_array.length(); ++l_i) {
    l_obj.emplace_back(in_array[l_i]);
  }

  std::vector<maya_poly_info> l_multimap{};
  l_multimap =
      l_obj | ranges::views::transform([](const MObject& in_object) { return maya_poly_info{in_object}; }) |
      ranges::views::filter([](const maya_poly_info& l_info) -> bool {
        return !l_info.maya_obj.isNull() && !l_info.is_intermediate_obj;
      }) |
      ranges::views::filter([](const maya_poly_info& l_info) -> bool { return l_info.has_cloth || l_info.has_skin; }) |
      ranges::to_vector;
  auto l_cloth = l_multimap |
                 ranges::views::filter([](const maya_poly_info& l_info) -> bool { return l_info.has_cloth; }) |
                 ranges::to_vector;
  auto l_skin =
      l_multimap |
      ranges::views::filter([](const maya_poly_info& l_info) -> bool { return l_info.has_skin && !l_info.has_cloth; }) |
      ranges::to_vector;

  duplicate_objs_ =
      l_cloth | ranges::views::transform([&](const maya_poly_info& in_object) -> std::pair<MObject, MObject> {
        auto l_v_list = l_skin | ranges::views::filter([&](const maya_poly_info& in_object_sk) -> bool {
                          return in_object == in_object_sk;
                        }) |
                        ranges::to_vector;
        return std::make_pair(l_v_list.empty() ? MObject{} : l_v_list.front().maya_obj, in_object.maya_obj);
      }) |
      ranges::views::filter([](const std::pair<MObject, MObject>& in_pair) -> bool {
        return !in_pair.first.isNull() && !in_pair.second.isNull();
      }) |
      ranges::to_vector;

  //  auto l_v = l_multimap |
  //             ranges::views::group_by(std::equal_to<maya_poly_info>{}) |
  //             ranges::to_vector;
  //  //  decltype(l_v)::value_type
  //  l_vector = l_v |
  //             ranges::views::filter([](const decltype(l_v)::value_type& in_vector) -> bool {
  //               return in_vector.size() > 1;
  //             }) |
  //             ranges::views::filter([this](const decltype(l_v)::value_type& in_vector) -> bool {
  //               return ranges::any_of(in_vector, [this](const maya_poly_info& in_info) -> bool {
  //                        return in_info.has_skin;
  //                      }) &&
  //                      ranges::any_of(in_vector, [this](const maya_poly_info& in_info) -> bool {
  //                        return in_info.has_cloth;
  //                      });
  //             }) |
  //             ranges::views::transform([this](const decltype(l_v)::value_type& in_vector)
  //                                          -> std::pair<MObject, MObject> {
  //               auto l_m_1 =
  //                   ranges::max_element(
  //                       in_vector,
  //                       [](const maya_poly_info& in_info_l,
  //                          const maya_poly_info& in_info_r) -> bool {
  //                         return in_info_l.skin_priority < in_info_r.skin_priority;
  //                       });
  //               auto l_m_2 =
  //                   ranges::max_element(
  //                       in_vector,
  //                       [](const maya_poly_info& in_info_l,
  //                          const maya_poly_info& in_info_r) -> bool {
  //                         return in_info_l.cloth_priority < in_info_r.cloth_priority;
  //                       });
  //               DOODLE_LOG_INFO("cloth_p {} skin {}", l_m_1->cloth_priority, l_m_1->skin_priority)
  //               DOODLE_LOG_INFO("cloth_p {} skin {}", l_m_2->cloth_priority, l_m_2->skin_priority)
  //
  //               return std::make_pair(l_m_1->maya_obj, l_m_2->maya_obj);
  //             }) |
  //             ranges::to_vector;

  return duplicate_objs_;
}
MObject find_duplicate_poly::operator[](const MObject& in_obj) {
  auto l_it = ranges::find_if(duplicate_objs_, [=](const std::pair<MObject, MObject>& in_par) -> bool {
    return in_obj == in_par.first || in_obj == in_par.second;
  });
  if (l_it == duplicate_objs_.end()) return {};

  return l_it->first == in_obj ? l_it->second : l_it->second;
}

}  // namespace doodle::maya_plug
