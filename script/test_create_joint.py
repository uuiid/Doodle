# -*- coding: utf-8 -*-
"""create_joint 的回归测试：两套同名骨骼必须各自拿到自己的关键帧。

背景（这是一次真实踩过的坑）:
    Maya 允许 anim_a|Hips 与 anim_b|Hips 这样同名但不同层级的节点共存。用短名调用
    cmds.setKeyframe / cmds.xform / cmds.select 只会命中其中一个，甚至在关节被
    cmds.parent 搬动后，之前拿到的路径字符串会失效直接报错——结果是第二套骨骼
    完全静止、一个关键帧都没有。因此 Skeleton 内部一律用完整 DAG 路径寻址，
    并把 root_group 存在实例上，避免每步手工传参时漏传。

运行方式:
    mayapy script/test_create_joint.py          # 命令行，自带 standalone 初始化
    或直接在 Maya Script Editor 里 execfile     # 复用当前场景

依赖 build 目录下已有的 json:
    res_dav.json / res_dav_2.json / send_dav.json / res_dav_settings.json
"""

import json
import math
import os
import sys


def _init_maya_cmds():
    """取到可用的 maya.cmds。

    mayapy 下 `import maya.cmds` 会成功但模块是空的（命令要等 standalone 初始化
    后才注册），所以用 hasattr(cmds, 'file') 判断是否真的可用；在 Maya 的 Script
    Editor 里则直接复用当前会话，不做初始化。
    """
    try:
        import maya.cmds as cmds
    except Exception:
        cmds = None

    if cmds is not None and hasattr(cmds, "file"):
        return cmds

    import maya.standalone

    maya.standalone.initialize(name="python")
    import maya.cmds as cmds

    return cmds


cmds = _init_maya_cmds()

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import create_joint  # noqa: E402

BUILD = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "build")
RES_A = os.path.join(BUILD, "res_dav.json")
RES_B = os.path.join(BUILD, "res_dav_2.json")
SEND = os.path.join(BUILD, "send_dav.json")
SETTINGS = os.path.join(BUILD, "res_dav_settings.json")

failures = []


def check(cond, msg):
    print(("  OK   " if cond else "  FAIL ") + msg)
    if not cond:
        failures.append(msg)


def key_count(joint_path):
    return cmds.keyframe(joint_path, q=True, keyframeCount=True) or 0


def load_json(path):
    with open(path, "r") as f:
        return json.load(f)


def expected_root_rot(response_path, frame_index):
    """按 Skeleton.apply_animation 的算法算出根关节该帧的 (rotateX, rotateY, rotateZ) 度数。

    用来验证某个根组下的骨骼拿到的确实是它自己那份 response 的数据。
    """
    resp = load_json(response_path)
    euler = create_joint.rotation_matrix_to_euler_xyz(resp["local_rot_mats"][frame_index][0])
    heading = math.atan2(
        resp["global_root_heading"][frame_index][1],
        resp["global_root_heading"][frame_index][0],
    )
    return (
        math.degrees(euler[0]),
        math.degrees(euler[1] + heading),
        math.degrees(euler[2]),
    )


def raises(exc_type, func, *args, **kwargs):
    """func(*args) 是否抛出 exc_type。"""
    try:
        func(*args, **kwargs)
    except exc_type:
        return True
    except Exception as e:  # 抛了别的异常也算不符合预期
        print(f"       (实际抛出 {type(e).__name__}: {e})")
        return False
    return False


def test_two_skeletons():
    """两段动画放进不同根组，各自的关键帧不能互相污染。"""
    print("=" * 70)
    print("两次调用，不同根组")
    cmds.file(new=True, force=True)

    ja = create_joint.create_animation_from_response(RES_A, SEND, root_group="anim_a")
    jb = create_joint.create_animation_from_response(RES_B, SETTINGS, root_group="anim_b")

    print()
    check(ja[0] == "|anim_a|Hips", f"anim_a 根路径 = {ja[0]}")
    check(jb[0] == "|anim_b|Hips", f"anim_b 根路径 = {jb[0]}")
    check(len(ja) == len(jb) == 30, f"两套骨骼各 30 关节 ({len(ja)}/{len(jb)})")
    check(all(p.startswith("|anim_a|") for p in ja.values()), "anim_a 关节都在 |anim_a 下")
    check(all(p.startswith("|anim_b|") for p in jb.values()), "anim_b 关节都在 |anim_b 下")

    # 核心回归点: 短名相同，但两套骨骼都要有关键帧
    for label, joints in (("anim_a", ja), ("anim_b", jb)):
        counts = [key_count(j) for j in joints.values()]
        check(min(counts) > 0, f"{label} 每个关节都有关键帧 (min={min(counts)})")

    # 关键帧归属: 各自的值必须来自各自的 response（这是"不互相污染"的直接证据）
    for label, sk, resp_path in (("anim_a", ja, RES_A), ("anim_b", jb, RES_B)):
        for frame_index in (0, 50, 143):
            frame = frame_index + 1
            exp_x, exp_y, exp_z = expected_root_rot(resp_path, frame_index)
            got_z = cmds.keyframe(sk[0], q=True, valueChange=True, time=(frame, frame),
                                  attribute="rotateZ")
            got_y = cmds.keyframe(sk[0], q=True, valueChange=True, time=(frame, frame),
                                  attribute="rotateY")
            got_z = got_z[0] if got_z else None
            got_y = got_y[0] if got_y else None
            ok = (
                got_z is not None
                and got_y is not None
                and abs(got_z - exp_z) < 1e-4
                and abs(got_y - exp_y) < 1e-4
            )
            check(ok, f"{label} frame {frame} 根 rotateY/Z 与自己的 response 一致 "
                      f"(期望 Z={exp_z:.3f} Y={exp_y:.3f}, 得到 Z={got_z} Y={got_y})")

    za = cmds.keyframe(ja[0], q=True, valueChange=True, time=(51, 51), attribute="rotateZ")[0]
    zb = cmds.keyframe(jb[0], q=True, valueChange=True, time=(51, 51), attribute="rotateZ")[0]
    check(abs(za - zb) > 1e-6, f"第 51 帧两套骨骼确实不同 (a={za:.4f}, b={zb:.4f})")

    # verify() 必须通过
    check(ja.verify() is True, "anim_a 校验通过")
    check(jb.verify() is True, "anim_b 校验通过")

    # 约束组: 取决于 send_dav.json 里有没有 constraint_lst
    if ja.constraint_lst:
        check(bool(cmds.ls("|anim_a|constraint_lst*")),
              f"anim_a 下有约束组 ({len(ja.constraint_lst)} 条约束)")
        con = cmds.listRelatives("|anim_a|constraint_lst", ad=True, type="joint",
                                 fullPath=True) or []
        check(len(con) == 30, f"约束骨骼 30 关节 ({len(con)})")
        check(sum(key_count(j) for j in con) == 0, "约束骨骼是静态姿态、无关键帧")
    else:
        check(not cmds.ls("|anim_a|constraint_lst*"),
              "数据里没有 constraint_lst，anim_a 下也就没有约束组")
    # settings 文件本来就没有 segment，不该有约束组
    check(not cmds.ls("|anim_b|constraint_lst*"), "anim_b 下没有约束组")


def test_rerun_reuses_group():
    """重复运行时复用已有根组，新骨骼仍要拿到自己的关键帧。"""
    print()
    print("=" * 70)
    print("同一根组重复运行")
    cmds.file(new=True, force=True)

    j1 = create_joint.create_animation_from_response(RES_A, SEND, root_group="anim_a")
    j2 = create_joint.create_animation_from_response(RES_A, SEND, root_group="anim_a")

    print()
    check(j1[0] == "|anim_a|Hips", f"第一次根路径 = {j1[0]}")
    # 同一父节点下同名时 Maya 会加数字后缀，这是正常的
    check(j2[0] == "|anim_a|Hips1", f"第二次根路径 = {j2[0]}")
    check(not (set(j1.values()) & set(j2.values())), "两次调用是两套互不重叠的骨骼")
    check(j1[1] == "|anim_a|Hips|Spine1", f"第一次子关节 = {j1[1]}")
    check(j2[1] == "|anim_a|Hips1|Spine1", f"第二次子关节 = {j2[1]}")
    check(min(key_count(j) for j in j1.values()) > 0, "第一次的关节都有 key")
    check(min(key_count(j) for j in j2.values()) > 0, "第二次的关节都有 key")


def test_skeleton_class():
    """Skeleton 类：数据存得住、链式调用能跑、守卫会拦。"""
    print()
    print("=" * 70)
    print("Skeleton 类")
    cmds.file(new=True, force=True)

    sk = create_joint.Skeleton.from_response_json(RES_A, SEND, root_group="anim_a")

    print()
    # --- 构造只准备数据，不碰 Maya ---
    check(sk.created is False, "构造后还没建骨骼 (created=False)")
    check(sk.joints == {}, "构造后 joints 为空")
    check(len(sk) == 30 and sk.joint_count == 30, f"len(sk)=joint_count={len(sk)}")
    check(sk.root_group == "anim_a", f"create() 前 root_group 是入参名 ({sk.root_group})")
    check(sk.num_frames is None and sk.fps is None, "还没打动画时 fps/num_frames 为 None")

    # --- 骨架数据存在实例上 ---
    check(sk.joint_name(0) == "Hips", f"joint_name(0) = {sk.joint_name(0)}")
    check(sk.parent_index(1) == 0, f"parent_index(1) = {sk.parent_index(1)} (Spine1 挂在 Hips 下)")
    check(len(sk.neutral_joint(0)) == 3, "neutral_joint(0) 是 3 维位置")
    check(sk.index_of_name("Hips") == 0, f"index_of_name('Hips') = {sk.index_of_name('Hips')}")
    check(sk.index_of_name("不存在的关节") is None, "未知名字返回 None")

    # --- 链式调用 ---
    sk.create().apply_animation().add_constraints().verify()

    print()
    check(sk.created is True, "create() 后 created=True")
    check(sk.root_group == "|anim_a", f"create() 后 root_group 变成完整路径 ({sk.root_group})")
    check(sk.joints[0] == "|anim_a|Hips", f"joints[0] = {sk.joints[0]}")
    check(sk[0] == sk.joint_path(0) == "|anim_a|Hips", "sk[0] / joint_path(0) 都是完整路径")
    check(sk.joint_path_by_name("Hips") == "|anim_a|Hips",
          f"joint_path_by_name('Hips') = {sk.joint_path_by_name('Hips')}")
    check(sk.joint_path_by_name("Spine1") == "|anim_a|Hips|Spine1",
          f"joint_path_by_name('Spine1') = {sk.joint_path_by_name('Spine1')}")
    check(sk.fps == 30, f"fps = {sk.fps}")
    expected_frames = len(load_json(RES_A)["local_rot_mats"])
    check(sk.num_frames == expected_frames, f"num_frames = {sk.num_frames} (数据 {expected_frames} 帧)")
    check(list(sk.keys()) == list(range(30)), "keys() 是 0..29")
    check(set(sk.values()) == set(sk.joints.values()), "values() 与 joints 一致")
    check(min(key_count(j) for j in sk.values()) > 0, "所有关节都有关键帧")

    # --- 守卫 ---
    print()
    check(raises(RuntimeError, sk.create), "重复 create() 抛 RuntimeError")

    fresh = create_joint.Skeleton.from_send_json(SEND, root_group="anim_c")
    check(raises(RuntimeError, fresh.apply_animation), "未 create() 就 apply_animation() 抛 RuntimeError")
    fresh.create()
    check(raises(RuntimeError, fresh.apply_animation), "没有动画数据时 apply_animation() 抛 RuntimeError")
    check(raises(KeyError, fresh.joint_path_by_name, "不存在的关节"), "未知关节名抛 KeyError")
    check(raises(KeyError, fresh.joint_path, 999), "越界下标抛 KeyError")

    # from_send_json 也应带上 constraint_lst（数量与文件一致，当前数据里是 0 条）
    expected_constraints = len(
        load_json(SEND).get("segment", {}).get("constraint_lst", [])
    )
    check(len(fresh.constraint_lst) == expected_constraints,
          f"from_send_json 读到的 constraint_lst 数量与文件一致 "
          f"({len(fresh.constraint_lst)} vs {expected_constraints})")

    # --- root_group=None 时落在世界层级 ---
    world = create_joint.Skeleton.from_send_json(SEND)
    world.create()
    check(world.root_group is None, "root_group=None 时仍是 None")
    check(world.joints[0] == "|Hips", f"关节落在世界层级 ({world.joints[0]})")

    # --- 约束姿态：当前数据里没有 constraint_lst，用合成数据覆盖这条路径 ---
    print()
    cmds.file(new=True, force=True)
    sk2 = create_joint.Skeleton.from_send_json(SEND, root_group="anim_d")
    sk2.create().apply_animation(load_json(RES_A))
    joint_total = sk2.joint_count
    sk2.add_constraints([
        {
            "type": "fullbody",
            "local_joints_rot": [[[0.0, 0.0, 0.0]] * joint_total],
            "root_positions": [[0.0, 0.0, 0.0]],
        }
    ])
    con = cmds.listRelatives("|anim_d|constraint_lst", ad=True, type="joint",
                             fullPath=True) or []
    check(len(con) == joint_total, f"合成约束建出 {len(con)} 个关节")
    check(all(p.startswith("|anim_d|constraint_lst|constraint_0|") for p in con),
          "约束骨骼都在 |anim_d|constraint_lst|constraint_0 下")
    check(sum(key_count(j) for j in con) == 0, "约束骨骼是静态姿态、无关键帧")
    check(min(key_count(j) for j in sk2.values()) > 0, "约束骨骼不影响动画骨骼的关键帧")


def test_resolve_joint_path():
    """resolve_joint_path 对短名的兼容解析。"""
    print()
    print("=" * 70)
    print("resolve_joint_path")
    cmds.file(new=True, force=True)
    create_joint.create_animation_from_response(RES_A, SEND, root_group="anim_a")
    create_joint.create_animation_from_response(RES_B, SETTINGS, root_group="anim_b")

    print()
    check(create_joint.resolve_joint_path("Hips", "anim_b") == "|anim_b|Hips",
          "短名 + 根组解析到正确层级")
    check(create_joint.resolve_joint_path("Hips", "|anim_b") == "|anim_b|Hips",
          "根组传完整路径也能解析")
    check(create_joint.resolve_joint_path("|anim_b|Hips") == "|anim_b|Hips",
          "完整路径原样返回")
    # 不传根组时同名关节无法区分，只能任取一个候选——所以调用方必须传根组
    ambiguous = create_joint.resolve_joint_path("Hips")
    check(ambiguous in ("|anim_a|Hips", "|anim_b|Hips"),
          f"不传根组时只能任取一个同名候选 ({ambiguous})，故必须传根组")


if __name__ == "__main__":
    test_two_skeletons()
    test_rerun_reuses_group()
    test_skeleton_class()
    test_resolve_joint_path()

    print()
    print("=" * 70)
    if failures:
        print(f"FAILED: {len(failures)}")
        for f in failures:
            print("  - " + f)
        sys.exit(1)
    print("ALL CHECKS PASSED")
