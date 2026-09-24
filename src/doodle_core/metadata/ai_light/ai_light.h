//
// AI 灯光：聚合头
//
// 实现 docs/ue_ai_lighting_plan.md。这一层只有数据结构，不含业务逻辑：
// 感知、Spec 蒸馏、候选生成、评判、轮次编排都在 doodle_lib 侧。
//
// 文件分工：
//   common.h     共享数值原语（interval_t）与 schema 版本常量
//   session.h    会话（= 方案里的 job）+ 会话状态机
//   reference.h  多张参考图，每张单独留统计（多图冲突要能回溯到具体哪张图）
//   spec.h       灯光目标 Spec（内存聚合体 + 表实体）
//   rig.h        灯光方案 LightingRig IR —— 唯一权威定义
//   iteration.h  轮次 + 人工判断
//   candidate.h  候选（每轮 6 个，含 rig 全量与显示空间指标）
//   preset.h     预设库
//
// 三条贯穿全层的约定：
//   1. 枚举按 magic_enum 的 C++ 名存 TEXT（sqlite_orm/orm/sqlite_statement.h:269-287）。
//      所以枚举项名必须与 JSON 字符串一致，且绝不重命名已有枚举项。
//   2. created_at_ / updated_at_ 的成员名与类型（chrono::system_zoned_time）必须一字不差，
//      ORM 靠它自动填充（sqlite_orm/orm/insert.h:95-98）。
//   3. 表实体是「查询权威」，嵌在 JSON 里的标识副本只用于工件自描述（下发 UE、落盘回归）。
//      写入时必须同源，见 ai_light_candidate::sync_from_rig()。
//

#pragma once

#include <doodle_core/metadata/ai_light/candidate.h>
#include <doodle_core/metadata/ai_light/common.h>
#include <doodle_core/metadata/ai_light/iteration.h>
#include <doodle_core/metadata/ai_light/preset.h>
#include <doodle_core/metadata/ai_light/reference.h>
#include <doodle_core/metadata/ai_light/rig.h>
#include <doodle_core/metadata/ai_light/session.h>
#include <doodle_core/metadata/ai_light/spec.h>
