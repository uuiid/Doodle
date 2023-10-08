//
// Created by td_main on 2023/10/8.
//

#include <maya/MDagPath.h>
#include <maya/MString.h>
#include <string>
namespace doodle::maya_plug::details {
struct cmp_dag {
  bool operator()(const MDagPath& lhs, const MDagPath& rhs) const {
    std::string const name1{lhs.fullPathName().asChar()};
    std::string const name2{rhs.fullPathName().asChar()};
    return (name1.compare(name2) < 0);
  }
};
}  // namespace doodle::maya_plug::details