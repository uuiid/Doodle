import maya.cmds as cmds


def _null(*args):
    pass


class _shelf():
    '''A simple class to build shelves in maya. Since the build method is empty,
    it should be extended by the derived class to build the necessary shelf elements.
    By default it creates an empty shelf called "customShelf".'''
    _instances = set()

    def __init__(self, name="customShelf", iconPath=""):
        self._instances.add(self)
        self.name = name

        self.iconPath = iconPath

        self.labelBackground = (0, 0, 0, 0)
        self.labelColour = (.9, .9, .9)

        self._cleanOldShelf()
        cmds.setParent(self.name)
        self.build()

    def build(self):
        '''This method should be overwritten in derived classes to actually build the shelf
        elements. Otherwise, nothing is added to the shelf.'''
        pass

    def addButon(self, label, icon="commandButton.png", command=_null, doubleCommand=_null):
        '''Adds a shelf button with the specified label, command, double click command and image.'''
        cmds.setParent(self.name)
        if icon:
            icon = self.iconPath + icon
        return cmds.shelfButton(width=37, height=37, image=icon, l=label, command=command, dcc=doubleCommand, imageOverlayLabel=label, olb=self.labelBackground, olc=self.labelColour)

    def addMenuItem(self, parent, label, command=_null, icon=""):
        '''Adds a shelf button with the specified label, command, double click command and image.'''
        if icon:
            icon = self.iconPath + icon
        return cmds.menuItem(p=parent, l=label, c=command, i="")

    def addSubMenu(self, parent, label, icon=None):
        '''Adds a sub menu item with the specified label and icon to the specified parent popup menu.'''
        if icon:
            icon = self.iconPath + icon
        return cmds.menuItem(p=parent, l=label, i=icon, subMenu=1)

    def _cleanOldShelf(self):
        '''Checks if the shelf exists and empties it if it does or creates it if it does not.'''
        if cmds.shelfLayout(self.name, ex=1):
            if cmds.shelfLayout(self.name, q=1, ca=1):
                for each in cmds.shelfLayout(self.name, q=1, ca=1):
                    cmds.deleteUI(each)
        else:
            cmds.shelfLayout(self.name, p="ShelfLayout")

    @staticmethod
    def create():
        _shelf.__init__("doodle")

    @staticmethod
    def deleteSelf():
        for inst in tuple(_shelf._instances):
            if cmds.shelfLayout(inst.name, ex=1):
                try:
                    cmds.deleteUI(inst.name)
                except RuntimeError:
                    pass
            _shelf._instances.discard(inst)

###################################################################################
'''This is an example shelf.'''
# class customShelf(_shelf):
#     def build(self):
#         self.addButon(label="button1")
#         self.addButon("button2")
#         self.addButon("popup")
#         p = cmds.popupMenu(b=1)
#         self.addMenuItem(p, "popupMenuItem1")
#         self.addMenuItem(p, "popupMenuItem2")
#         sub = self.addSubMenu(p, "subMenuLevel1")
#         self.addMenuItem(sub, "subMenuLevel1Item1")
#         sub2 = self.addSubMenu(sub, "subMenuLevel2")
#         self.addMenuItem(sub2, "subMenuLevel2Item1")
#         self.addMenuItem(sub2, "subMenuLevel2Item2")
#         self.addMenuItem(sub, "subMenuLevel1Item2")
#         self.addMenuItem(p, "popupMenuItem3")
#         self.addButon("button3")
# customShelf()
###################################################################################
