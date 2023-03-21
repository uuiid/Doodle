//
// Created by TD on 2022/3/8.
//

#include <main/maya_plug_fwd.h>

namespace doodle::maya_plug {

namespace {
constexpr char find_duplicate_poly_comm_name[] = "doodle_duplicate_poly";
}
/**
 * @brief maya 命令语法 现在只有 -na -namespace 来指定寻找的名称空间
 * @return maya 命令语法类
 */
MSyntax find_duplicate_poly_comm_syntax();
/**
 * @brief 使用 maya obj 对 执行 @b qlUpdateInitialPose 命令
 * 传入的命令查看 find_duplicate_poly_comm_syntax
 * 当传入一个名称空间时只会进行选中,不会执行命令, 用来debug
 *
 */
class find_duplicate_poly_comm
    : public TemplateAction<find_duplicate_poly_comm, find_duplicate_poly_comm_name, find_duplicate_poly_comm_syntax> {
 public:
  MStatus doIt(const MArgList& in_list) override;
};

}  // namespace doodle::maya_plug
