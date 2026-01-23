#include "doodle_batch_run.h"

#include "doodle_core/exception/exception.h"

#include "data/maya_conv_str.h"
#include <maya/MArgDatabase.h>
namespace doodle::maya_plug {

MSyntax doodle_batch_run_syntax() {
  static constexpr auto cloth_sim_config{"-cloth_sim"};
  static constexpr auto export_fbx_config{"-export_fbx"};
  static constexpr auto replace_file_config{"-replace_file"};
  static constexpr auto inspect_file_config{"-inspect_file"};
  static constexpr auto export_rig_config{"-export_rig"};
  MSyntax syntax;
  syntax.addFlag("-c", "-config", MSyntax::kString);
  syntax.addFlag("-clsm", cloth_sim_config, MSyntax::kString);
  syntax.addFlag("-efbx", export_fbx_config, MSyntax::kString);
  syntax.addFlag("-ref", replace_file_config, MSyntax::kString);
  syntax.addFlag("-inf", inspect_file_config, MSyntax::kString);
  syntax.addFlag("-erig", export_rig_config, MSyntax::kString);
  return syntax;
}

class doodle_batch_run::impl {
 public:
  impl() {}
};

MStatus doodle_batch_run::doIt(const MArgList& in_list) {
  try {
    ptr = std::make_unique<impl>();
    MArgDatabase arg_data(syntax(), in_list);
    auto l_config_str = arg_data.flagArgumentString("-config", 0);
    DOODLE_CHICK(l_config_str.length() == 0, "没有传入正确的配置文件路径");
    nlohmann::json l_json = nlohmann::json::parse(FSys::ifstream{FSys::from_quotation_marks(conv::to_s(l_config_str))});
    

  } catch (const doodle_error& e) {
    setResult(e.error_code_);
    displayError(conv::to_ms(e.what()));
    return MStatus::kSuccess;
  } catch (...) {
    setResult(-1);
    displayError(conv::to_ms(boost::current_exception_diagnostic_information()));
    return MStatus::kSuccess;
  }
  return MStatus::kSuccess;
}

}  // namespace doodle::maya_plug