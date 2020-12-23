import json
import re
import os
import pymel.core
import pymel.all

import sys
# import inspect
# __Doodle__ = os.path.dirname(inspect.getsourcefile(lambda: 0))
# __Doodle__ = os.path.dirname(os.path.dirname(__Doodle__))
# print("add path {}".format(__Doodle__))
# sys.path.append(__Doodle__)
import argsTools

print("{} {}".format(argsTools.ARGS.path,
                     argsTools.ARGS.exportpath))
# maya.cmds.file(new=True, force=True)
# maya.cmds.file(os.path.join(args.path), o=True)

if os.path.exists(os.path.dirname(argsTools.ARGS.path) + "/workspace.mel"):
    pymel.all.workspace.open(os.path.dirname(argsTools.ARGS.path))
    pymel.all.workspace.save()

pymel.core.newFile(force=True, type="mayaAscii")
pymel.core.openFile(os.path.abspath(argsTools.ARGS.path), force=True)


class chickFile():
    def __init__(self):
        pass

    def selectAllPolygons(self):
        self.geometrys = pymel.core.ls(geometry=True)

    def chickUvMap(self):
        for geo in self.geometrys:
            pymel.core.polyUVSet(geo, allUvSet=True)

    def __call__(self):
        self.selectAllPolygons()
        self.chickUvMap()
