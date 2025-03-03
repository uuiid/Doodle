//
// Created by TD on 25-2-28.
//

#include "fbx_file.h"

#include <fbxsdk.h>
namespace doodle::fbx {
std::vector<std::string> get_all_materials(const FSys::path& in_path, bool in_split_namespace) {
  std::vector<std::string> l_materials{};
  const auto l_manager =
      std::shared_ptr<FbxManager>(FbxManager::Create(), [](FbxManager* in_ptr) { in_ptr->Destroy(); });
  l_manager->SetIOSettings(FbxIOSettings::Create(l_manager.get(), IOSROOT));

  FbxScene* l_scene = FbxScene::Create(l_manager.get(), "doodle_to_ue_fbx");
  if (const auto l_importer = std::shared_ptr<FbxImporter>(
          FbxImporter::Create(l_manager.get(), ""), [](FbxImporter* in_ptr) { in_ptr->Destroy(); }
      );
      l_importer->Initialize(in_path.generic_string().c_str(), -1, l_manager->GetIOSettings()))
    l_importer->Import(l_scene);
  else
    throw_exception(doodle_error{"打开文件 {} 错误 {}", in_path, l_importer->GetStatus().GetErrorString()});

  auto l_root = l_scene->GetRootNode();
  if (!l_root) return l_materials;

  std::function<void(const FbxNode*, std::vector<std::string>*)> l_add_mats{};
  l_add_mats = [&l_add_mats](const FbxNode* in_node, std::vector<std::string>* in_mats) {
    for (auto i = 0; i < in_node->GetChildCount(); ++i) {
      auto l_node = in_node->GetChild(i);
      if (!l_node) continue;
      for (auto j = 0; j < l_node->GetMaterialCount(); ++j) {
        auto l_mat = l_node->GetMaterial(j);
        if (!l_mat) continue;
        if (auto l_mat_name = l_mat->GetName()) in_mats->emplace_back(l_mat_name);
      }

      l_add_mats(l_node, in_mats);
    }
  };

  l_add_mats(l_root, &l_materials);
  l_materials |= ranges::actions::unique | ranges::actions::sort;
  if (in_split_namespace)
    for (auto&& i: l_materials) {
      i = i.substr(i.find_last_of(':') + 1);
    }
  return l_materials;
}
}  // namespace doodle::fbx