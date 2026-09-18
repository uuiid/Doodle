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
        r'E:\Doodle\build\send_dav.json',
        root_group='anim_b')
注意: 同名关节在第二次创建时会被 Maya 自动加数字后缀 (Hips1, Spine11, ...)，
按组名区分两套骨骼即可 (如 'anim_a|Hips' 与 'anim_b|Hips')。

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
    """获取或创建收纳一套骨骼的根组（空变换组），返回实际组名。

    root_group 为空 (None / '') 时返回 None，表示不建组、直接放在世界层级。
    组已存在时直接复用，便于脚本重复运行而不产生嵌套组。
    """
    if not root_group:
        return None
    if cmds.objExists(root_group):
        print(f"  reusing existing root group '{root_group}'")
        return root_group
    group = cmds.group(empty=True, name=root_group)
    print(f"  created root group '{group}'")
    return group


def parent_joints_to_root_group(created, skeleton_data, root_group):
    """把所有根关节 (parent_idx < 0) 挂到 root_group 下，保持世界坐标不变。"""
    if not root_group:
        return
    root_joints = [
        created[i]
        for i, joint_def in enumerate(skeleton_data)
        if joint_def["parent_idx"] < 0
    ]
    if not root_joints:
        return
    # 默认不带 -relative，Maya 会补偿局部矩阵以保持世界变换
    cmds.parent(root_joints, root_group)


def create_joints_from_skeleton(skeleton_data, root_group=None):
    """根据 send_dav.json 的 skeleton 列表创建 Maya 关节层级，返回 {index: joint_name}。

    root_group: 非空时，创建完成后把根关节挂到该组下。传入不同的根组即可在同一
                场景中创建多套骨骼（如两段动画的对比）。
    """
    created = {}
    for i, joint_def in enumerate(skeleton_data):
        name = joint_def["name"]
        pos = joint_def["neutral_joint"]
        parent_idx = joint_def["parent_idx"]

        cmds.select(clear=True)
        if parent_idx >= 0 and parent_idx in created:
            cmds.select(created[parent_idx])

        jnt = cmds.joint(name=name, position=(pos[0], pos[1], pos[2]))
        created[i] = jnt

    # 将关节 orient 归零，以便直接用旋转矩阵驱动
    for i, jnt in created.items():
        cmds.setAttr(f"{jnt}.jointOrientX", 0)
        cmds.setAttr(f"{jnt}.jointOrientY", 0)
        cmds.setAttr(f"{jnt}.jointOrientZ", 0)
        # 设置 radius 为 0.02，便于在 Maya 中查看
        cmds.setAttr(f"{jnt}.radius", 0.02)

    parent_joints_to_root_group(created, skeleton_data, root_group)

    return created


def apply_animation_from_response(created_joints, response_data):
    """将 motion_output 数据作为关键帧动画应用到已创建的关节上。"""
    # posed_joints = response_data["posed_joints"]            # [T, J, 3]
    local_rot_mats = response_data["local_rot_mats"]  # [T, J, 3, 3]
    smooth_root_pos = response_data["smooth_root_pos"]  # [T, 3]
    global_root_heading = response_data["global_root_heading"]  # [T, 2]
    fps = response_data.get("fps", 30.0)
    fps = int(fps)

    num_frames = len(local_rot_mats)
    num_joints = len(local_rot_mats[0])

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
        root_jnt = created_joints[0]
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
            jnt = created_joints[j]
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
    constraint_group = cmds.group(empty=True, name="constraint_lst")

    for c, constraint in enumerate(constraint_lst):
        sub_group = cmds.group(empty=True, name=f"constraint_{c}")
        cmds.parent(sub_group, constraint_group)

        created = {}
        for i, joint_def in enumerate(skeleton_data):
            name = joint_def["name"]
            pos = joint_def["neutral_joint"]
            parent_idx = joint_def["parent_idx"]

            cmds.select(clear=True)
            if parent_idx >= 0 and parent_idx in created:
                cmds.select(created[parent_idx])

            jnt = cmds.joint(name=name, position=(pos[0], pos[1], pos[2]))
            created[i] = jnt

        for i, jnt in created.items():
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

        cmds.parent(created[0], sub_group)
        print(f"  constraint {c} ({constraint.get('type')}) created")

    if root_group:
        cmds.parent(constraint_group, root_group)


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

    返回值仍为 created_joints ({index: joint_name})，与不传 root_group 时一致。
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
    apply_animation_from_response(created_joints, response_data)

    create_constraint_joints(send_dav_json, skeleton_data, group)

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
