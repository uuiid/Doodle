import json
import maya.cmds as cmds


def create_joints_from_json(json_path):
    with open(json_path, 'r') as f:
        data = json.load(f)

    skeleton = data['skeleton']
    bone_names = skeleton['bone_order_names']
    joint_parents = skeleton['joint_parents']
    neutral_joints = skeleton['neutral_joints']

    created_joints = {}

    for i, name in enumerate(bone_names):
        pos = neutral_joints[i]
        parent_idx = joint_parents[i]

        cmds.select(clear=True)

        if parent_idx >= 0 and parent_idx in created_joints:
            cmds.select(created_joints[parent_idx])

        joint_name = cmds.joint(name=name, position=(pos[0], pos[1], pos[2]))
        created_joints[i] = joint_name

    print(f"Created {len(created_joints)} joints.")


if __name__ == '__main__':
    create_joints_from_json(r'E:\Doodle\build\joint.json')