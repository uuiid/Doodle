//
// Created by TD on 25-7-21.
//
#pragma once

#include <doodle_lib/ai/skeleton/skeleton_base.h>
#include <doodle_lib/core/global_function.h>

#include <Eigen/Dense>
#include <cstdint>
#include <vector>

// ======================================================================
// 已废弃 — FK 和 global_rots_to_local_rots 现已作为成员方法
// 位于 doodle::ai::skeleton_base 中。
// 请使用 skel->fk(...) 和 skel->global_rots_to_local_rots(...) 代替。
// 此头文件保留仅用于向后兼容。
// ======================================================================
