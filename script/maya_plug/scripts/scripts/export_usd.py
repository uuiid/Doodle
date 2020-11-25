import pymel.core
import re
import random
import os


def export():
    export = pymel.core.ls(sl=True)
    comMwsh = {}

    mesh_globe = []
    pymel.core.hyperShade(shaderNetworksSelectMaterialNodes=True)
    for shd in pymel.core.selected(materials=True):
        if [c for c in shd.classification() if 'shader/surface' in c]:
            comMwsh[shd] = []

    for mesh in [m.getShape() for m in export]:
        if getMat(mesh.getTransform()).__len__() > 1:
            for inst in mesh.instObjGroups:
                for iGrp in inst.objectGroups:
                    try:
                        pymel.core.polyChipOff([mesh.node() + "." + tmp_ for tmp_ in iGrp.objectGrpCompList.get()],
                                                duplicate=False, keepFacesTogether=True)
                    except:
                        pass

            splitnodeList = pymel.core.polySeparate(mesh.getTransform())
            for spM in splitnodeList:
                pymel.core.select(spM)
                pymel.core.hyperShade(shaderNetworksSelectMaterialNodes=True)
                for shd in pymel.core.selected(materials=True):
                    if [c for c in shd.classification() if 'shader/surface' in c]:
                        comMwsh[shd].append(spM)
        else:
            try:
                comMwsh[getMat(mesh.getTransform())[0]].append(mesh.getTransform())
            except IndexError:
                print("{} is not materal".format(mesh.getTransform()))
    for key, merg in comMwsh.items():
        if merg.__len__() > 1:
            mesh_globe.append(pymel.core.polyUnite(merg, name="{}_export".format(key.name()))[0])
        else:
            mesh_globe.append(merg[0])
    exportUsd(mesh_globe)


def getMat(obj):
    pymel.core.select(obj, replace=True)
    pymel.core.hyperShade(shaderNetworksSelectMaterialNodes=True)
    mat = []
    for shd in pymel.core.selected(materials=True):
        if [c for c in shd.classification() if 'shader/surface' in c]:
            mat.append(shd)
    return mat


def exportUsd(exportList):
    pymel.core.select(exportList, replace=True)
    root = pymel.core.group(name=exportList[0].name().split(":")[0].split("_")[0] + random.randint(1, 100).__str__(),
                            world=True)
    # for i in exportList:
    #     pymel.core.parent(i,root)

    filename = pymel.core.system.sceneName()
    name_parsing_ep = re.findall("ep\d+", filename)
    name_parsing_shot = re.findall("sc\d+[_BCD]", filename)
    _eps = -1
    _shot = -1
    _shotab = ""
    start = pymel.core.playbackOptions(query=True, min=True)
    end = pymel.core.playbackOptions(query=True, max=True)
    if name_parsing_ep and name_parsing_shot:
        try:
            _eps = int(name_parsing_ep[0][2:])
        except NameError:
            _eps = 1
        try:
            _shot = int(name_parsing_shot[0][2:-1])
            shotab = name_parsing_shot[0][-1:]
            if shotab != "_":
                _shotab = shotab
            else:
                _shotab = ""
        except NameError:
            _shot = 1
            _shotab = ""
    try:
        depp = re.split("[A,a]nm", filename)[-1].split("_")[1]
    except:
        depp = None
    name_e = "shot_ep{ep:0>3d}_sc{sh:0>4d}{shab}_VFX_usd_{name_}_.{s}_{e}.usd".format(ep=_eps, sh=_shot, shab=_shotab,
                                                                                      name_=
                                                                                      root.name().split(":")[0].split(
                                                                                          "_")[0],
                                                                                      s=start, e=end)
    pymel.core.mayaUSDExport(file="D:/03_Workflow/shots/ep{ep:0>3d}/sc{sh:0>4d}{shab}/Anm/{de}/{na}".format(ep=_eps,
                                                                                                            sh=_shot,
                                                                                                            shab=_shotab,
                                                                                                            de=depp,
                                                                                                            na=name_e),
                             selection=True,
                             exportUVs=True,
                             exportColorSets=True,
                             defaultUSDFormat="usdc",
                             eulerFilter=True,
                             frameRange=(start, end))
    os.startfile("D:/03_Workflow/shots/ep{:0>3d}/sc{:0>4d}{}/Anm/{}/".format(_eps,
                                                                             _shot,
                                                                             _shotab,
                                                                             depp))
