import maya.cmds as cmd


def doodle_has_FK_joint(in_joint):
    l_names = in_joint.split(":")
    l_names[-1] = "FK{}".format(l_names[-1])
    l_str = ":".join(l_names)
    try:
        cmd.select(l_str)
    except ValueError:
        # print("not select {}".format(l_str))
        return False
    return True


def doodle_get_FK_joint(in_joint):
    l_names = in_joint.split(":")
    l_names[-1] = "FK{}".format(l_names[-1])
    cmd.select(":".join(l_names))
    l_select = cmd.ls(sl=1)

    return l_select[0]


def doodle_create_locator(in_joint):
    l_list = cmd.spaceLocator()[0]
    cmd.parentConstraint(in_joint, l_list, dr=0)
    return l_list


def set_fk_constraint(in_joint, in_loc):
    cmd.parentConstraint(in_loc, in_joint, dr=0)
    return in_joint


def doodle_ik_to_fk():
    cmd.select(cmd.ls(sl=1)[0], hi=1)
    l_j_list = cmd.ls(sl=1)

    l_j_list = [i for i in l_j_list if cmd.nodeType(i) == "joint"]
    l_j_list = filter(doodle_has_FK_joint, l_j_list)

    l_map_j_fk = {i: doodle_get_FK_joint(i) for i in l_j_list}
    l_loc_list = [doodle_create_locator(i) for i in l_j_list]
    cmd.bakeResults(l_loc_list, t=(int(cmd.playbackOptions(
        query=True, min=True)), int(cmd.playbackOptions(query=True, max=True))))
    cmd.delete(l_loc_list, cn=1)
    l_fk_list = [set_fk_constraint(l_map_j_fk[j], l_loc_list[i])
                 for i, j in enumerate(l_j_list)]
    cmd.bakeResults(l_fk_list, t=(int(cmd.playbackOptions(
        query=True, min=True)), int(cmd.playbackOptions(query=True, max=True))))
    cmd.delete(l_fk_list, cn=1)
    cmd.delete(l_loc_list)
