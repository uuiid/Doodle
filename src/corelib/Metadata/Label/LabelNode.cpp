//
// Created by TD on 2021/5/6.
//

#include "LabelNode.h"
#include <corelib/Metadata/Assets.h>
#include <entt/entt.hpp>
#include <entt/signal/sigh.hpp>

namespace doodle {
LabelNode::LabelNode() {
}

void LabelNode::test() {
  entt::registry test;
  auto entity  = test.create();
  auto &k_item = test.emplace<LabelNode>(entity);

  auto &k_item1 = test.get<LabelNode>(entity);
  auto k_item2 = test.on_update<LabelNode>();

  test.on_construct<LabelNode>().connect<&LabelNode::test>();
//  test.on_construct<LabelNode>().connect<&LabelNode::test>(this);
  entt::collector.update<LabelNode>().where<LabelNode>();
  test.on_construct<LabelNode>();
  test.destroy(entity);
}

}  // namespace doodle
