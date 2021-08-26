import shelfBase
import maya.cmds as cmds

import scripts.Doodle_PolyRemesh as Doodle_PolyRemesh
import scripts.Doodle_exportUe as Doodle_exportUe
import scripts.Doodle_cam as Doodle_cam
import scripts.Doodle_clear as Doodle_clear
import scripts.Doodle_dem_bone as Doodle_dem_bone
import scripts.Doodle_deleteSurplusWeight as deleteWeight
import scripts.Doodle_deleteAttr as deleteAttr
import scripts.export_usd as export_usd

from PySide2 import QtCore
from PySide2 import QtGui
from PySide2 import QtWidgets

import pymel.core
import pymel.core.system


class DlsShelf(shelfBase._shelf):
    cloth_to_fbx = None

    # def __init__(self, name="customShelf", iconPath=""):
    #     super(DlsShelf,self).__init__(name, iconPath)
    #     self.cloth_to_fbx = None

    def build(self):
        # self.addButon("export_cam", icon="icons/OUTcam.png", command=self.exportCam)
        self.addButon("export_abc", icon="icons/OUTabc.png",
                      command=self.exportAbc)
        self.addButon("back_cam", icon="icons/back_cam.png",
                      command=self.BakeAimCam)

        self.addButon("remesh", icon="icons/remesh.png",
                      command=self.polyremesh)

        self.addButon("clear", icon="icons/clear.png", command=self.clearScane)
        self.addButon("CLoth_to_Fbx", icon="icons/cloth_to_fbx.png",
                      command=self.clothToFbx)
        self.addButon("delect Weight", icon="icons/ue_delete_weight.png",
                      command=self.deleteWeightPoint)
        self.addButon("delect Mixed deformation attr", icon="icons/doodle_delete_attr",
                      command=self.deleteAttr)
        self.addButon("export usd", icon="icons/export_usd.png",
                      command=self.exportUSD)
        self.addButon("repair", icon="icons/repair", command=self.repair)
        self.addButon("randomColor", icon="icons/randomColor.png",
                      command=self.randomColor)
        self.addButon("mark_sim", icon="icons/mark_sim.png",
                      command=self.mark_sim)

    def polyremesh(self):
        self.re()
        Doodle_PolyRemesh.myRemesh()

    def exportCam(self):
        self.re()
        Doodle_exportUe.exportUe().export("one")

    def exportAbc(self):
        self.re()
        Doodle_exportUe.exportUe().export("two")

    def BakeAimCam(self):
        self.re()
        Doodle_cam.camBakeAim()

    def clearScane(self):
        self.re()
        Doodle_clear.clearAndUpload().clearScane()

    def clothToFbx(self):
        self.re()
        if self.cloth_to_fbx:
            self.cloth_to_fbx.show()
        else:
            self.cloth_to_fbx = Doodle_dem_bone.DleClothToFbx().show()

    def deleteWeightPoint(self):
        self.re()
        deleteWeight.deleteSurplusWeight().show()

    def deleteAttr(self):
        self.re()
        deleteAttr.deleteShape().show()

    def exportUSD(self):
        self.re()
        export_usd.export()

    def repair(self):
        import pymel.core
        import re
        f = pymel.core.listReferences()
        for i in f:
            print(i)
            if (re.match(r"V:/03_Workflow/Assets/[P,p]rop", i.__str__())):
                if (re.match(r"V:/03_Workflow/Assets/[P,p]rops", i.__str__())):
                    continue
                try:
                    i.load(re.sub(
                        r"V:/03_Workflow/Assets/[P,p]rop", r"V:/03_Workflow/Assets/props", i.__str__()))
                except:
                    pass

    def randomColor(self):
        import pymel.core
        import random
        select_lists = pymel.core.ls(sl=True)
        for select_obj in select_lists:
            pymel.core.select(select_obj)
            pymel.core.polyColorPerVertex(colorDisplayOption=True,
                                          rgb=(random.random() * .5, random.random() * .5, random.random() * .5))

    def mark_sim(self):
        select = pymel.core.selected()
        fls = set()  # type: set[pymel.core.system.FileReference]
        for s in select:
            ref = pymel.core.referenceQuery(s, referenceNode=True, topReference=True)
            fls.add(pymel.core.FileReference(ref))
        print(fls)
        pymel.core.system.fileInfo["doodle_sim"] = str(fls)

    def re(self):
        key = QtWidgets.QApplication.keyboardModifiers()
        if key == QtCore.Qt.ShiftModifier:
            reload(Doodle_PolyRemesh)
            reload(Doodle_exportUe)
            reload(Doodle_cam)
            reload(Doodle_clear)
            reload(Doodle_dem_bone)
            reload(deleteWeight)
            reload(deleteAttr)
            reload(export_usd)


class DoodleUIManage(object):
    _instances = set()

    @staticmethod
    def creation():
        shelf = DlsShelf("Doodle")
        DoodleUIManage._instances.add(shelf)

    @staticmethod
    def deleteSelf():
        for inst in tuple(DoodleUIManage._instances):
            if cmds.shelfLayout(inst.name, ex=1):
                try:
                    cmds.deleteUI(inst.name)
                except RuntimeError:
                    pass
            DoodleUIManage._instances.discard(inst)
