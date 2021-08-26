import maya.cmds
import pymel.core


class clearAndUpload(object):
    def __init__(self):
        self.not_nu_plug = ["mayaHlK"]

    def clearScane(self):
        un_know_plugs = pymel.core.unknownPlugin(query=True, list=True)
        if un_know_plugs:
            for un_plug in un_know_plugs:
                if un_plug in self.not_nu_plug:
                    continue
                print("Remove unknown plugin {}".format(un_plug))
                pymel.core.unknownPlugin(un_plug, remove=True)

        plugs = pymel.core.pluginInfo(query=True, listPlugins=True)
        if plugs:
            for plug in plugs:
                if plug in self.not_nu_plug:
                    continue
                print("Cancel{} Automatic loading of plug-in writing".format(plug))
                pymel.core.pluginInfo(plug, edit=True, writeRequires=False)
        print("=" * 30 + "clear ok" + "=" * 30)

        self.calar_vaccine_gene()

    def calar_vaccine_gene(self):
        for s in pymel.core.ls(type="script"):
            if s.name() == "vaccine_gene" or s.name() == "breed_gene":
                pymel.core.delete(s)
                path = pymel.core.Path(
                    pymel.core.internalVar(userAppDir=True) + "scripts")
                for f in path.files():
                    f.remove_p()
