//
// Created by TD on 2022/3/8.
//

#include <maya_plug_fwd.h>

namespace doodle::maya_plug {

namespace {
constexpr char find_duplicate_poly_comm_name[] = "doodle_duplicate_poly";
}
MSyntax find_duplicate_poly_comm_syntax();
class find_duplicate_poly_comm : public TemplateAction<find_duplicate_poly_comm,
                                                       find_duplicate_poly_comm_name,
                                                       find_duplicate_poly_comm_syntax> {
 public:
  virtual MStatus doIt(const MArgList& in_list) override;
};

}  // namespace doodle::maya_plug
