//
// Created by TD on 2021/12/16.
//

#pragma once
#include <maya/MApiNamespace.h>
#include <string>

namespace doodle::maya_plug {
/**
 * @brief 这个插件会寻找节点的属性, 并在找到时返回
 * @param in_node 节点obj
 * @param in_name 属性名称
 * @return 找到的属性类
 *
 * 这个函数会先寻找节点本身的属性, 如果没有找到, 并且节点下方有 形状 节点, 还会扩展到形状节点中检查属性
 */
MPlug get_plug(const MObject& in_node, const std::string& in_name);

MObject get_shading_engine(const MObject& in_node);

MObject get_first_mesh(const MObject& in_node);
}  // namespace doodle::maya_plug
