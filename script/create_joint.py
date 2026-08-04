"""
从 motion_output JSON 创建 Maya 骨骼动画。

用法: 在 Maya Script Editor 中运行:
    import sys
    sys.path.insert(0, r'E:\Doodle\script')
    import create_joint
    create_joint.create_animation_from_response(
        r'E:\Doodle\build\response2.json',
        r'E:\Doodle\build\joint.json'
    )

两个 JSON 来源:
  - response2.json: kimodo::generate() 返回的 motion_output 序列化结果
  - joint.json:     骨骼定义 (bone_order_names, joint_parents, neutral_joints)
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


def create_joints_from_skeleton(skeleton_data):
    """根据骨骼定义创建 Maya 关节层级，返回 {index: joint_name}。"""
    bone_names = skeleton_data["bone_order_names"]
    joint_parents = skeleton_data["joint_parents"]
    neutral_joints = skeleton_data["neutral_joints"]

    created = {}
    for i, name in enumerate(bone_names):
        pos = neutral_joints[i]
        parent_idx = joint_parents[i]

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

    return created


def apply_animation_from_response(created_joints, response_data):
    """将 motion_output 数据作为关键帧动画应用到已创建的关节上。"""
    posed_joints = response_data["posed_joints"]            # [T, J, 3]
    local_rot_mats = response_data["local_rot_mats"]        # [T, J, 3, 3]
    smooth_root_pos = response_data["smooth_root_pos"]      # [T, 3]
    global_root_heading = response_data["global_root_heading"]  # [T, 2]
    fps = response_data.get("fps", 30.0)
    fps = int(fps)

    num_frames = len(posed_joints)
    num_joints = len(posed_joints[0])

    cmds.currentUnit(time=f"{fps}fps")
    start_frame = 1

    cmds.playbackOptions(
        minTime=start_frame,
        maxTime=start_frame + num_frames - 1,
        animationStartTime=start_frame,
        animationEndTime=start_frame + num_frames - 1
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
        cmds.xform(root_jnt, ws=False, ro=(math.degrees(root_euler[0]),
                                            math.degrees(root_euler[1] + heading_angle),
                                            math.degrees(root_euler[2])))

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
            cmds.xform(jnt, ws=False, ro=(math.degrees(euler[0]),
                                           math.degrees(euler[1]),
                                           math.degrees(euler[2])))
            cmds.setKeyframe(jnt, attribute="rotateX")
            cmds.setKeyframe(jnt, attribute="rotateY")
            cmds.setKeyframe(jnt, attribute="rotateZ")

        if t % 20 == 0:
            print(f"  keyed frame {frame}/{start_frame + num_frames - 1}")

    print(f"Animation applied: {num_frames} frames, {num_joints} joints")


def create_animation_from_response(response_json_path, skeleton_json_path):
    """主入口：从两个 JSON 文件创建完整的骨骼动画。"""
    with open(skeleton_json_path, "r") as f:
        skeleton_json = json.load(f)
    skeleton_data = skeleton_json["skeleton"]

    with open(response_json_path, "r") as f:
        response_data = json.load(f)

    print(f"Creating {len(skeleton_data['bone_order_names'])} joints...")
    created_joints = create_joints_from_skeleton(skeleton_data)

    print(f"Applying animation ({len(response_data['posed_joints'])} frames)...")
    apply_animation_from_response(created_joints, response_data)

    print("Done.")
    return created_joints


# 保留旧接口兼容
def create_joints_from_json(json_path):
    """旧接口：仅创建骨骼，不添加动画。"""
    with open(json_path, "r") as f:
        data = json.load(f)
    return create_joints_from_skeleton(data["skeleton"])


if __name__ == "__main__":
    create_animation_from_response(
        r"E:\Doodle\build\response2.json",
        r"E:\Doodle\build\joint.json"
    )