#include <src/seting.h>
#include <src/Project.h>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem.hpp>
DOODLE_NAMESPACE_S

Seting::Seting() {
}

Seting& Seting::Get() noexcept {
  static Seting instance;
  return instance;
}

void Seting::init() const {
  auto& prj = Project::Get();

  nlohmann::json root;

  auto k_config = boost::dll::program_location().parent_path() / "config" / "config.json";
  if (fileSys::exists(k_config)) {
    fileSys::ifstream file{k_config, std::ios::in};
    if (file.is_open()) {
      root = nlohmann::json::parse(file);
      prj.from_json(root["project"]);
    }
  } else {
    root["test"]           = "W:\\";
    root["DuBuXiaoYao_3"]  = "V:\\";
    root["changanhuanjie"] = "X:\\";
    root["KuangShenMoZun"] = "T:\\";
    root["WanYuFengShen"]  = "U:\\";
    prj.from_json(root);
  }
}

Seting::~Seting() {
}
DOODLE_NAMESPACE_E