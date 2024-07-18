//
// Created by TD on 2024/2/27.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::http {
/**
 * @brief task_info 任务信息
 * 提交任务时,需要json格式
 * key:
 * data: 任务数据
 * source_computer: 任务来源计算机
 * submitter: 提交者
 * name: 任务名称
 * 回复为任务 id
 *
 * 任务数据的基本格式
 * 自动灯光任务:
 * type: auto_light_task
 * file_path: maya路径
 * export_anim_time: 导出动画时间
 * episodes: 集数
 * shot: 镜头
 * shot_enum: 镜头枚举(可选)
 * project_name: 项目名称
 * project_path: 项目路径
 * project_en_str: 项目英文缩写
 * project_shor_str: 项目简称
 */

void task_info_reg(http_route& in_route);
} // namespace doodle::http