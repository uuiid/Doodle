from maya import cmds
from maya import OpenMaya
import os


def create_root_uv_attribute(curves_group, mesh_node, uv_set='map1'):
    '''
    Create "groom_root_uv" attribute on group of curves.
    '''

    # check curves group
    if not cmds.objExists(curves_group):
        raise RuntimeError('Group not found: "{}"'.format(curves_group))

    # get curves in group
    curve_shapes = cmds.listRelatives(curves_group, shapes=True, noIntermediate=True)
    curve_shapes = cmds.ls(curve_shapes, type='nurbsCurve')
    if not curve_shapes:
        raise RuntimeError('Invalid curves group. No nurbs-curves found in group.')
    else:
        print("found curves")
        # print curve_shapes

    # get curve roots
    points = list()
    for curve_shape in curve_shapes:
        point = cmds.pointPosition('{}.cv[0]'.format(curve_shape), world=True)
        points.append(point)

    # get uvs
    values = list()
    uvs = find_closest_uv_point(points, mesh_node, uv_set=uv_set)
    for u, v in uvs:
        values.append([u, v, 0])
        #print (str(u) + " , " + str(v)  )

    # create attribute
    name = 'groom_root_uv'
    if name in cmds.listAttr(curves_group):
        cmds.deleteAttr('{}.{}'.format(curves_group, name))
        cmds.deleteAttr('{}.{}_AbcGeomScope'.format(curves_group, name))
        cmds.deleteAttr('{}.{}_AbcType'.format(curves_group, name))

    cmds.addAttr(curves_group, ln=name, dt='vectorArray')
    cmds.addAttr(curves_group, ln='{}_AbcGeomScope'.format(name), dt='string')
    cmds.addAttr(curves_group, ln='{}_AbcType'.format(name), dt='string')

    cmds.setAttr('{}.{}'.format(curves_group, name), len(values), *values, type='vectorArray')
    cmds.setAttr('{}.{}_AbcGeomScope'.format(curves_group, name), 'uni', type='string')
    cmds.setAttr('{}.{}_AbcType'.format(curves_group, name), 'vector2', type='string')

    return uvs

def find_closest_uv_point(points, mesh_node, uv_set='map1'):
    '''
    Find mesh UV-coordinates at given points.
    '''

    # check mesh
    if not cmds.objExists(mesh_node):
        raise RuntimeError('Node not found: "{}"'.format(mesh_node))

    # check uv_set
    uv_sets = cmds.polyUVSet(mesh_node, q=True, allUVSets=True)
    if uv_set not in uv_sets:
        raise RuntimeError('Invalid uv_set provided: "{}"'.format(uv_set))

    # get mesh as dag-path
    selection_list = OpenMaya.MSelectionList()
    selection_list.add(mesh_node)

    mesh_dagpath = OpenMaya.MDagPath()
    selection_list.getDagPath(0, mesh_dagpath)
    mesh_dagpath.extendToShape()

    # get mesh function set
    fn_mesh = OpenMaya.MFnMesh(mesh_dagpath)

    uvs = list()
    for i in range(len(points)):

        script_util = OpenMaya.MScriptUtil()
        script_util.createFromDouble(0.0, 0.0)
        uv_point = script_util.asFloat2Ptr()

        point = OpenMaya.MPoint(*points[i])
        fn_mesh.getUVAtPoint(point, uv_point, OpenMaya.MSpace.kWorld, uv_set)

        u = OpenMaya.MScriptUtil.getFloat2ArrayItem(uv_point, 0, 0)
        v = OpenMaya.MScriptUtil.getFloat2ArrayItem(uv_point, 0, 1)

        uvs.append((u, v))

    return uvs

def abc_export(filepath, node=None, start_frame=1, end_frame=1, data_format='otawa', uv_write=True):

    job_command = '-frameRange {} {} '.format(start_frame, end_frame)
    job_command += '-dataFormat {} '.format(data_format)

    job_command += '-attr groom_root_uv '

    if uv_write:
        job_command += '-uvWrite '

    job_command += '-root {} '.format(node)

    job_command += '-file {} '.format(filepath)

    cmds.AbcExport(verbose=True, j=job_command)




def main():

    export_directory = 'C:/'
    hair_file = os.path.join(export_directory, 'hair_export.abc')
    l_select = cmds.ls(sl=1)
    if len(l_select) != 2:
        cmds.error("Select two objects according to UV model and hair curve")
        return
    uv_mesh=l_select[0]
    curve_top_group= l_select[1]
    l_out = cmds.fileDialog2(fileFilter="*.abc" )
    if not l_out:
        return
    l_out = l_out[0]
    create_root_uv_attribute( curve_top_group , uv_mesh)
    abc_export(l_out, curve_top_group)
