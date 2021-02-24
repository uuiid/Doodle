/*
 * @Author: your name
 * @Date: 2020-09-18 17:14:11
 * @LastEditTime: 2020-12-14 13:31:13
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\core\CoreData.h
 */
#pragma once

#include <corelib/core_global.h>
#include <corelib/core/coreset.h>
#include <rttr/type>

DOODLE_NAMESPACE_S

namespace pathParser {
class PathParser;
}

class CORE_API CoreData {
  RTTR_ENABLE();

 protected:
  std::vector<std::shared_ptr<fileSys::path>> p_roots;

 public:
  CoreData();
  virtual bool setInfo(const std::string &value) = 0;

  // template <typename T, typename U>
  // static std::vector<std::shared_ptr<T>> getAll(const std::shared_ptr<U> &core_data_ptr);

  DOODLE_DISABLE_COPY(CoreData)

  const dpathListPtr &Roots() const noexcept;
  void setRoots(const dpathListPtr &Roots) noexcept;
};

// template <typename T, typename U>
// static std::vector<std::shared_ptr<T>> CoreData::getAll(const std::shared_ptr<U> &core_data_ptr) {
//   auto &set = coreSet::getSet();

//   auto roots       = core_data_ptr->Roots();
//   auto path_parser = set.getProject()->findParser(rttr::type::get<T>());

//   std::vector<std::shared_ptr<T>> ass_list{};
//   if (roots.size() == path_parser.size()) {
//     for (size_t i = 0; i < roots.size(); ++i) {
//       auto lists = path_parser[i]->parser(*roots[i]);
//       for (auto &&item : lists) {
//         if (item.get().can_convert(rttr::type::get<T>())) {
//           auto &ass = item.get().get_value<T>();
//           ass_list.push_back(ass.shared_from_this());
//         }
//       }
//     }
//   }
//   return ass_list;
// };

DOODLE_NAMESPACE_E
