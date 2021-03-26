import maya.cmds


class clearAndUpload(object):
    def __init__(self):
        self.not_nu_plug = ["mayaHlK"]

    def clearScane(self):
        un_know_plugs = maya.cmds.unknownPlugin(query=True, list=True)
        if un_know_plugs:
            for un_plug in un_know_plugs:
                if un_plug in self.not_nu_plug:
                    continue
                maya.cmds.unknownPlugin(un_plug, remove=True)

        plugs = maya.cmds.pluginInfo(query=True, listPlugins=True)
        if plugs:
            for plug in plugs:
                if plug in self.not_nu_plug:
                    continue
                maya.cmds.pluginInfo(plug, edit=True, writeRequires=False)
        print("=" * 30 + "clear ok" + "=" * 30)
