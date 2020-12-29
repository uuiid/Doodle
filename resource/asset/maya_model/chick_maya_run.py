# -*- coding: utf-8 -*-
import argsTools
import chick_maya_model
import os
import pymel.core

# import inspect
# __Doodle__ = os.path.dirname(inspect.getsourcefile(lambda: 0))
# __Doodle__ = os.path.dirname(os.path.dirname(__Doodle__))
# print("add path {}".format(__Doodle__))
# sys.path.append(__Doodle__)


print("file path {}\nexport path {}".format(argsTools.ARGS.path,
                                            argsTools.ARGS.exportpath))

workspace = os.path.dirname(os.path.dirname(argsTools.ARGS.path))
if os.path.exists(workspace + "/workspace.mel"):
    pymel.all.workspace.open(workspace)
    pymel.all.workspace.save()
    print("set workspace {}".format(workspace))

pymel.core.newFile(force=True, type="mayaAscii")
pymel.core.openFile(os.path.abspath(argsTools.ARGS.path), force=True)

chick_maya_model.run()(argsTools.DLOG)
