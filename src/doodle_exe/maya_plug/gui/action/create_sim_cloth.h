//
// Created by TD on 2021/12/20.
//

#pragma once
namespace doodle::maya_plug {
class create_sim_cloth : public process_t<create_sim_cloth> {
  std::vector<entt::handle> p_list;

  entt::handle p_coll;

 public:
  create_sim_cloth();
  ~create_sim_cloth();

  constexpr static std::string_view name{"创建布料"};

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(delta_type, void* data);

  bool render();
};
}  // namespace doodle::maya_plug
