# -*- coding: utf-8 -*-
"""create_joint 的回归测试：两套同名骨骼必须各自拿到自己的关键帧。

背景（这是一次真实踩过的坑）:
    Maya 允许 anim_a|Hips 与 anim_b|Hips 这样同名但不同层级的节点共存。用短名调用
    cmds.setKeyframe / cmds.xform / cmds.select 只会命中其中一个，甚至在关节被
    cmds.parent 搬动后，之前拿到的路径字符串会失效直接报错——结果是第二套骨骼
    完全静止、一个关键帧都没有。因此 create_joint 内部一律用完整 DAG 路径寻址。

运行方式:
    mayapy script/test_create_joint.py          # 命令行，自带 standalone 初始化
    或直接在 Maya Script Editor 里 execfile     # 复用当前场景

依赖 build 目录下已有的 json:
    res_dav.json / res_dav_2.json / send_dav.json / res_dav_settings.json
"""

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

    # 关键帧归属: 各自的值必须来自各自的 response
    za = cmds.keyframe(ja[0], q=True, valueChange=True, time=(51, 51), attribute="rotateZ")[0]
    zb = cmds.keyframe(jb[0], q=True, valueChange=True, time=(51, 51), attribute="rotateZ")[0]
    check(abs(za - zb) > 1e-6, f"第 51 帧两套骨骼确实不同 (a={za:.4f}, b={zb:.4f})")

    # verify_animation 必须通过
    check(create_joint.verify_animation(ja, "|anim_a") is True, "anim_a 校验通过")
    check(create_joint.verify_animation(jb, "|anim_b") is True, "anim_b 校验通过")

    # send_dav.json 有 constraint_lst -> anim_a 下有约束组；settings 没有 -> anim_b 下没有
    check(bool(cmds.ls("|anim_a|constraint_lst*")), "anim_a 下有约束组")
    check(not cmds.ls("|anim_b|constraint_lst*"), "anim_b 下没有约束组")
    con = cmds.listRelatives("|anim_a|constraint_lst", ad=True, type="joint",
                             fullPath=True) or []
    check(len(con) == 30, f"约束骨骼 30 关节 ({len(con)})")
    check(sum(key_count(j) for j in con) == 0, "约束骨骼是静态姿态、无关键帧")


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
    test_resolve_joint_path()

    print()
    print("=" * 70)
    if failures:
        print(f"FAILED: {len(failures)}")
        for f in failures:
            print("  - " + f)
        sys.exit(1)
    print("ALL CHECKS PASSED")
