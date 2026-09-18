r"""
从 motion_output JSON 创建 Maya 骨骼动画。

用法: 在 Maya Script Editor 中运行:
    import sys
    sys.path.insert(0, r'E:\Doodle\script')
    import create_joint
    create_joint.create_animation_from_response(
        r'E:\Doodle\build\response2.json',
        r'E:\Doodle\build\send_dav.json'
    )

对比两段动画（重定向结果是否有区别）: 每次调用传入不同的 root_group，
每套骨骼会被收纳到各自的根组下，可在同一场景中并排查看:
    create_joint.create_animation_from_response(
        r'E:\Doodle\build\res_dav.json',
        r'E:\Doodle\build\send_dav.json',
        root_group='anim_a')
    create_joint.create_animation_from_response(
        r'E:\Doodle\build\res_dav_2.json',
        r'E:\Doodle\build\res_dav_settings.json',
        root_group='anim_b')

重要: Maya 允许 anim_a|Hips 与 anim_b|Hips 这样同名但不同层级的节点同时存在，
此时用短名调用 xform / setKeyframe / parent 只会命中其中一个（通常是先创建的
那套），结果是第二套骨骼完全静止、关键帧全打在上一套上。因此本脚本所有关节都
按完整 DAG 路径 (|anim_b|Hips|Spine1) 寻址，create_joints_from_skeleton 返回的
也是完整路径；apply_animation_from_response 必须传入对应的 root_group。

两个 JSON 来源:
  - response2.json: kimodo 生成接口返回的动画数据 (local_rot_mats, smooth_root_pos, global_root_heading)
  - send_dav.json: 请求数据，其 skeleton 字段定义骨骼 (name, parent_idx, neutral_joint)
"""

import json
import math
import maya.cmds as cmds


def rotation_matrix_to_euler_xyz(m):
    """将 3×3 旋转矩阵转换为 Maya 默认 XYZ 旋转顺序的欧拉角 (弧度)。"""
    r00, r01, r02 = m[0][0], m[0][1], m[0][2]
    r10, r11, r12 = m[1][0], m[1][1], m[1][2]
    r20, r21, r22 = m[2][0], m[2][1], m[2][2]

    if r20 < 1.0 - 1e-6:
        if r20 > -1.0 + 1e-6:
            ry = -math.asin(r20)
            cos_y = math.cos(ry)
            rx = math.atan2(r21 / cos_y, r22 / cos_y)
            rz = math.atan2(r10 / cos_y, r00 / cos_y)
        else:
            ry = math.pi / 2.0
            rx = -math.atan2(-r12, r11)
            rz = 0.0
    else:
        ry = -math.pi / 2.0
        rx = math.atan2(-r12, r11)
        rz = 0.0

    return rx, ry, rz


def axis_angle_to_rotation_matrix(v):
    """轴角向量 → 3×3 旋转矩阵（行主序，与 geometry.cpp 的 Rodrigues 一致）。"""
    x, y, z = v[0], v[1], v[2]
    angle = math.sqrt(x * x + y * y + z * z)
    if angle < 1e-6:
        # 小角度近似 R ≈ I + skew(v)
        return [
            [1.0, -z, y],
            [z, 1.0, -x],
            [-y, x, 1.0],
        ]
    x /= angle
    y /= angle
    z /= angle
    c = math.cos(angle)
    s = math.sin(angle)
    omc = 1.0 - c
    ksq_00 = -y * y - z * z
    ksq_01 = x * y
    ksq_02 = x * z
    ksq_10 = x * y
    ksq_11 = -x * x - z * z
    ksq_12 = y * z
    ksq_20 = x * z
    ksq_21 = y * z
    ksq_22 = -x * x - y * y
    return [
        [1.0 + omc * ksq_00, -z * s + omc * ksq_01, y * s + omc * ksq_02],
        [z * s + omc * ksq_10, 1.0 + omc * ksq_11, -x * s + omc * ksq_12],
        [-y * s + omc * ksq_20, x * s + omc * ksq_21, 1.0 + omc * ksq_22],
    ]


def get_or_create_root_group(root_group):
    """获取或创建收纳一套骨骼的根组（空变换组），返回完整路径（如 '|anim_a'）。

    root_group 为空 (None / '') 时返回 None，表示不建组、直接放在世界层级。
    组已存在时直接复用，便于脚本重复运行而不产生嵌套组。
    """
    if not root_group:
        return None
    if cmds.objExists(root_group):
        print(f"  reusing existing root group '{root_group}'")
        return to_full_path(root_group)
    full = to_full_path(cmds.group(empty=True, name=root_group))
    print(f"  created root group '{full}'")
    return full


def to_full_path(node):
    """把 Maya 返回的节点引用规范化成完整 DAG 路径。

    Maya 创建节点时返回值不固定：名字唯一时返回短名 ('Hips')，与已有同名节点
    冲突时返回部分路径 ('anim_b|Hips')。两种都要转成 '|anim_b|Hips' 才能唯一定位。
    """
    if not node:
        return node
    if node.startswith("|"):
        return node
    if "|" in node:
        return "|" + node  # 部分路径，补上世界根

    matches = cmds.ls(node, long=True) or []
    if not matches:
        return node
    # 同名节点可能有多个，优先取世界层级（路径里只有一个 '|'）的那个
    for match in matches:
        if match.count("|") == 1:
            return match
    return matches[0]


def joint_full_path(parent_path, created_name):
    """由唯一已知的父路径 + Maya 返回的名字拼出精确的完整路径。

    created_name 可能是 'Hips' 或 'anim_b|Hips'，只取最后一段短名即可：
    父路径本身就是唯一的，因此拼接结果一定指向刚创建的那个关节。
    """
    return f"{parent_path}|{created_name.rsplit('|', 1)[-1]}"


def resolve_joint_path(jnt, root_group=None):
    """把关节引用解析成完整 DAG 路径，保证同名不同层级的骨骼不会被选错。

    已经是完整路径（以 '|' 开头）时原样返回；否则用 cmds.ls 找出全部同名节点，
    优先取 root_group 下的那个，其次取世界层级的那个。
    """
    if not jnt or jnt.startswith("|"):
        return jnt

    candidates = cmds.ls(jnt, long=True) or []
    if not candidates:
        return jnt

    if root_group:
        prefix = to_full_path(root_group) + "|"
        for candidate in candidates:
            if candidate.startswith(prefix):
                return candidate

    # 没有根组时优先取世界层级的节点（完整路径里只有一个 '|'）
    for candidate in candidates:
        if candidate.count("|") == 1:
            return candidate

    return candidates[0]


def create_joints_from_skeleton(skeleton_data, root_group=None):
    """根据 send_dav.json 的 skeleton 列表创建 Maya 关节层级。

    root_group: 非空时把根关节直接建在该空组下（组不存在则新建）。传入不同的根组
                即可在同一场景中创建多套骨骼（如两段动画的对比）。这里是完整路径。

    返回 {index: 完整 DAG 路径}，例如 {0: '|anim_a|Hips', 1: '|anim_a|Hips|Spine1'}。
    返回完整路径是必须的：两套骨骼的同名关节位于不同层级，只有完整路径才能唯一定位。
    """
    root_parent = root_group or ""

    created = {}
    for i, joint_def in enumerate(skeleton_data):
        name = joint_def["name"]
        pos = joint_def["neutral_joint"]
        parent_idx = joint_def["parent_idx"]

        # 父节点一律用完整路径选择：短名会在两套同名骨骼中命中先创建的那一套，
        # 导致第二套骨骼被挂到第一套下面
        if parent_idx is not None and parent_idx >= 0 and parent_idx in created:
            parent_path = created[parent_idx]
        else:
            parent_path = root_parent

        cmds.select(clear=True)
        if parent_path:
            cmds.select(parent_path)

        jnt = cmds.joint(name=name, position=(pos[0], pos[1], pos[2]))
        created[i] = joint_full_path(parent_path, jnt)

    # 将关节 orient 归零，以便直接用旋转矩阵驱动
    for jnt in created.values():
        cmds.setAttr(f"{jnt}.jointOrientX", 0)
        cmds.setAttr(f"{jnt}.jointOrientY", 0)
        cmds.setAttr(f"{jnt}.jointOrientZ", 0)
        # 设置 radius 为 0.02，便于在 Maya 中查看
        cmds.setAttr(f"{jnt}.radius", 0.02)

    return created


def apply_animation_from_response(created_joints, response_data, root_group=None):
    """将 motion_output 数据作为关键帧动画应用到已创建的关节上。

    root_group: 这套骨骼所属的根组，必须传入。Maya 中 anim_a|Hips 与 anim_b|Hips
                同名但层级不同，用短名调用 xform / setKeyframe 会默认命中先创建的
                那套骨骼，表现为第二套骨骼完全静止、关键帧全打在上一套上。这里先把
                所有关节统一解析成完整 DAG 路径，再按路径操作。
    """
    # posed_joints = response_data["posed_joints"]            # [T, J, 3]
    local_rot_mats = response_data["local_rot_mats"]  # [T, J, 3, 3]
    smooth_root_pos = response_data["smooth_root_pos"]  # [T, 3]
    global_root_heading = response_data["global_root_heading"]  # [T, 2]
    fps = response_data.get("fps", 30.0)
    fps = int(fps)

    num_frames = len(local_rot_mats)
    num_joints = len(local_rot_mats[0])

    # 同名关节必须带层级访问，否则会操作到另一套骨骼上
    joints = {i: resolve_joint_path(jnt, root_group)
              for i, jnt in created_joints.items()}

    cmds.currentUnit(time=f"{fps}fps")
    start_frame = 1

    cmds.playbackOptions(
        minTime=start_frame,
        maxTime=start_frame + num_frames - 1,
        animationStartTime=start_frame,
        animationEndTime=start_frame + num_frames - 1,
    )

    for t in range(num_frames):
        frame = start_frame + t
        cmds.currentTime(frame)

        # --- 根关节 (Hips, index=0) ---
        root_jnt = joints[0]
        sp = smooth_root_pos[t]
        cmds.xform(root_jnt, ws=True, t=(sp[0], sp[1], sp[2]))

        # Y 轴朝向
        heading_cos, heading_sin = global_root_heading[t][0], global_root_heading[t][1]
        heading_angle = math.atan2(heading_sin, heading_cos)

        root_rot_mat = local_rot_mats[t][0]
        root_euler = rotation_matrix_to_euler_xyz(root_rot_mat)
        cmds.xform(
            root_jnt,
            ws=False,
            ro=(
                math.degrees(root_euler[0]),
                math.degrees(root_euler[1] + heading_angle),
                math.degrees(root_euler[2]),
            ),
        )

        cmds.setKeyframe(root_jnt, attribute="translateX")
        cmds.setKeyframe(root_jnt, attribute="translateY")
        cmds.setKeyframe(root_jnt, attribute="translateZ")
        cmds.setKeyframe(root_jnt, attribute="rotateX")
        cmds.setKeyframe(root_jnt, attribute="rotateY")
        cmds.setKeyframe(root_jnt, attribute="rotateZ")

        # --- 其他关节 ---
        for j in range(1, num_joints):
            jnt = joints[j]
            rot_mat = local_rot_mats[t][j]
            euler = rotation_matrix_to_euler_xyz(rot_mat)
            cmds.xform(
                jnt,
                ws=False,
                ro=(
                    math.degrees(euler[0]),
                    math.degrees(euler[1]),
                    math.degrees(euler[2]),
                ),
            )
            cmds.setKeyframe(jnt, attribute="rotateX")
            cmds.setKeyframe(jnt, attribute="rotateY")
            cmds.setKeyframe(jnt, attribute="rotateZ")

        if t % 20 == 0:
            print(f"  keyed frame {frame}/{start_frame + num_frames - 1}")

    print(f"Animation applied: {num_frames} frames, {num_joints} joints")


def create_constraint_joints(send_dav_json, skeleton_data, root_group=None):
    """将 segment.constraint_lst 创建为约束姿态骨骼，置于 constraint_lst 组下。

    root_group: 非空时，constraint_lst 组也会挂到该根组下，使多套骨骼的约束
                姿态随各自的动画一起分组、互不混淆。
    """
    segment = send_dav_json.get("segment", {})
    constraint_lst = segment.get("constraint_lst", [])
    if not constraint_lst:
        return

    cmds.select(clear=True)
    constraint_group = to_full_path(cmds.group(empty=True, name="constraint_lst"))

    for c, constraint in enumerate(constraint_lst):
        sub_group = to_full_path(cmds.group(empty=True, name=f"constraint_{c}"))
        cmds.parent(sub_group, constraint_group)

        # 约束骨骼直接建在子组下，路径从创建起就唯一，无需事后搬动根关节；
        # 注意子组刚被搬到 constraint_lst 下，路径要按新的父路径重算
        sub_parent = joint_full_path(constraint_group, sub_group)

        created = {}
        for i, joint_def in enumerate(skeleton_data):
            name = joint_def["name"]
            pos = joint_def["neutral_joint"]
            parent_idx = joint_def["parent_idx"]

            if parent_idx is not None and parent_idx >= 0 and parent_idx in created:
                parent_path = created[parent_idx]
            else:
                parent_path = sub_parent

            cmds.select(clear=True)
            if parent_path:
                cmds.select(parent_path)

            jnt = cmds.joint(name=name, position=(pos[0], pos[1], pos[2]))
            created[i] = joint_full_path(parent_path, jnt)

        for jnt in created.values():
            cmds.setAttr(f"{jnt}.jointOrientX", 0)
            cmds.setAttr(f"{jnt}.jointOrientY", 0)
            cmds.setAttr(f"{jnt}.jointOrientZ", 0)
            cmds.setAttr(f"{jnt}.radius", 0.02)

        # 姿态：根位移 + 各关节局部旋转（轴角，与 constraint_set.cpp 的 from_dict 一致）
        local_joints_rot = constraint["local_joints_rot"]
        root_positions = constraint["root_positions"]
        frame_rot = local_joints_rot[0]
        root_pos = root_positions[0]

        root_jnt = created[0]
        cmds.xform(root_jnt, ws=True, t=(root_pos[0], root_pos[1], root_pos[2]))

        for j, jnt in created.items():
            rot_mat = axis_angle_to_rotation_matrix(frame_rot[j])
            euler = rotation_matrix_to_euler_xyz(rot_mat)
            cmds.xform(
                jnt,
                ws=False,
                ro=(
                    math.degrees(euler[0]),
                    math.degrees(euler[1]),
                    math.degrees(euler[2]),
                ),
            )

        print(f"  constraint {c} ({constraint.get('type')}) created")

    if root_group:
        cmds.parent(constraint_group, root_group)


def verify_animation(created_joints, root_group=None):
    """校验骨骼确实挂在目标根组下、且每个关节都拿到了关键帧。

    同名关节在不同层级共存时，Maya 按短名选择会命中另一套骨骼（或因为路径失效
    直接报错中断），表现为某个根组下的骨骼"完全静止、没有关键帧"。这里显式检查，
    把静默失败变成可见告警。
    """
    problems = []

    if root_group:
        prefix = to_full_path(root_group) + "|"
        for jnt in created_joints.values():
            if not jnt.startswith(prefix):
                problems.append(f"'{jnt}' 不在根组 '{root_group}' 下")
                break

    missing = [
        jnt
        for jnt in created_joints.values()
        if not (cmds.keyframe(jnt, q=True, keyframeCount=True) or 0)
    ]
    if missing:
        problems.append(f"{len(missing)} 个关节没有关键帧 (前几个: {missing[:3]})")

    if problems:
        print("  WARNING: " + "; ".join(problems))
        return False

    print(f"  verified: {len(created_joints)} 个关节都在目标根组下且已打关键帧")
    return True


def create_animation_from_response(
    response_json_path, send_dav_json_path, root_group=None
):
    """主入口：从 send_dav.json 的 skeleton 字段创建骨骼，从 response JSON 应用动画。

    root_group: 非空时创建/复用该空组，本次调用产生的骨骼与约束姿态都收纳在其中。
                对不同的 response JSON 传入不同的根组，即可在同一场景中生成多套
                骨骼动画，用于对比两段动画（如两次重定向结果）是否有区别。
                组不存在时按传入名新建，因此实际组名恒等于 root_group。

    注意: 每次调用都会设置场景时间单位与播放范围；若两段动画帧数不同，播放范围
          以最后一次调用为准（已烘焙的关键帧不受影响，可直接拖动时间轴查看）。

    返回 {index: 完整 DAG 路径}，例如 {0: '|anim_a|Hips'}。必须是完整路径：
    anim_a|Hips 与 anim_b|Hips 同名不同层级，只有完整路径能唯一定位。
    """
    with open(send_dav_json_path, "r") as f:
        send_dav_json = json.load(f)
    skeleton_data = send_dav_json["skeleton"]

    with open(response_json_path, "r") as f:
        response_data = json.load(f)

    group = get_or_create_root_group(root_group)

    print(
        f"Creating {len(skeleton_data)} joints..."
        + (f" under '{group}'" if group else "")
    )
    created_joints = create_joints_from_skeleton(skeleton_data, group)

    print(f"Applying animation ({len(response_data['local_rot_mats'])} frames)...")
    apply_animation_from_response(created_joints, response_data, group)

    create_constraint_joints(send_dav_json, skeleton_data, group)

    verify_animation(created_joints, group)

    print(f"Done{f' (root group: {group})' if group else ''}.")
    return created_joints


if __name__ == "__main__":
    # 对比两段动画：分别放入不同的根组，可在同一场景中并排查看重定向结果
    create_animation_from_response(
        r"E:\Doodle\build\res_dav.json",
        r"E:\Doodle\build\send_dav.json",
        root_group="anim_a",
    )
    create_animation_from_response(
        r"E:\Doodle\build\res_dav_2.json",
        r"E:\Doodle\build\res_dav_settings.json",  # 发送的数据中, 不存在骨骼, 所以使用的是标准骨骼, 直接使用setting 获取的骨骼
        root_group="anim_b",
    )
