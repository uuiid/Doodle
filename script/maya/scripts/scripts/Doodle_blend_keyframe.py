from maya import cmds
#exception
class My_Exception(RuntimeError):
    pass

#Gets the namespace of the object selected by the user
class user_select():
    def __init__(self):
        in_obj=cmds.ls(sl=True)
        self.circle_namespace=cmds.ls(in_obj,sns=True,r=True)
        self.namespace=self.circle_namespace[1]

    def __repr__(self):
        return self.namespace

    def __str__(self):
        return self.namespace   

#The operation of the maya node
class maya_node(object):
    def __init__(self, in_obj_name):
        if in_obj_name :
            obj=cmds.ls(in_obj_name)
            num=len(obj)
            if obj and num==1:
                self.name = cmds.ls(in_obj_name)[0]
        else:
            raise My_Exception('The object name is empty or the object name does not exist or has a duplicate name ')
  
    def connection(self, in_source):
        pass
    
    #Disconnect
    def delete_connection(self, in_target_plug):
        # Node1.a -> Node2.b
        # Node1.a -> Node3.b
        # source -> target
        # self
        # todo:  dsa
        if cmds.connectionInfo(in_target_plug, isDestination=True):
            source = cmds.connectionInfo(
                in_target_plug,sourceFromDestination=True)
            if source.find(self.name):
                self.dc=cmds.disconnectAttr(source, in_target_plug)
            else:
                raise My_Exception('source and target is a one-to-many relationship')

    def delete_node(self):
        self.delete=cmds.delete(self)

    def __repr__(self):
        return self.name

    def __str__(self):
        return self.name

#Locator
class Locator(maya_node):
    def __init__(self):
        try:
            super(maya_node,None)
        except My_Exception:
            cmds.warning('Instead of initializing the maya node, it initializes the locator')
        #Create a locator
        self.name = cmds.spaceLocator(p=(0, 0, 0))[0]
        self.__parent__ = None
        pass
    #Set locator properties

    @property
    def parent(self):
        return self.__parent__

    @parent.setter
    def parent(self, in_parent):
        self.__parent__ = in_parent
        if in_parent:
            cmds.parent(self.name, in_parent)
        else:
            cmds.parent(self.name, w=True)

    @property
    def translateX(self):
        return 0

    @translateX.setter
    def translateX(self, in_value):
        cmds.setAttr('%s.translateX' % self, in_value)

    @property
    def translateY(self):
        return 0

    @translateY.setter
    def translateY(self, in_value):
        cmds.setAttr('%s.translateY' % self, in_value)

    @property
    def translateZ(self):
        return 0

    @translateZ.setter
    def translateZ(self, in_value):
        cmds.setAttr('%s.translateZ' % self, in_value)
        pass

    @property
    def rotateX(self):
        return 0

    @rotateX.setter
    def rotateX(self, in_value):
        cmds.setAttr('%s.rotateX' % self, in_value)

    @property
    def rotateY(self):
        return 0

    @rotateY.setter
    def rotateY(self, in_value):
        cmds.setAttr('%s.rotateY' % self, in_value)

    @property
    def rotateZ(self):
        return 0

    @rotateZ.setter
    def rotateZ(self, in_value):
        cmds.setAttr('%s.rotateZ' % self, in_value)

    def __repr__(self):
        return self.name

    def __str__(self):
        return self.name

#Parent-child constraints
class parent_constraint(object):
    def __init__(self, in_c, in_p, in_mo):
        self.name = cmds.parentConstraint(
            in_c, in_p, mo=in_mo,dr=False, w=1.0)[0]

    def __del__(self):
        cmds.delete(self.name)

    def __repr__(self):
        return self.name

    def __str__(self):
        return self.name

#Bake animation
class back_anim(object):
    def __init__(self, in_back):
        #Get the start time
        self.start_time = cmds.playbackOptions(query=True, ast=True)
        #Get the end time
        self.end_time = cmds.playbackOptions(query=True, aet=True)
        self.result = cmds.bakeResults(in_back, t=(self.start_time, self.end_time),
                                       dic=True, pok=True, mr=True, ral=False, rba=False, bol=False, simulation=True)

    def __repr__(self):
        return self.result

    def __str__(self):
        return self.result

#Manage parent-child constrained files
class constraint_mg:
    def __init__(self, in_list, mo):
        self.constraint = [parent_constraint(
            item[0], item[1], in_mo=mo) for item in in_list]

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.constraint = None

#Operation process
def backeProcess():
    arr_locator = [Locator() for i in range(0, 5)]
    Circle_namespace=user_select().namespace
    arr_Circle = [maya_node(in_obj_name=Circle_namespace+':Main'),
                maya_node(in_obj_name=Circle_namespace+':main_con'),
                maya_node(in_obj_name=Circle_namespace+':RootX_M'),
                maya_node(in_obj_name=Circle_namespace+':FKRoot_M')]
    # Move the locator to the appropriate parent-child relationship
    arr_locator[1].parent = arr_locator[0]
    arr_locator[2].parent = arr_locator[1]
    arr_locator[3].parent = arr_locator[2]
    # Parent-child constraints are placed on locators and objects, and constraint files are deleted after baking
    with constraint_mg(zip(arr_Circle, arr_locator[0:4]), False) as f1:
        back_anim(arr_locator[0:4])
    # move locator 5 to the same level as locator 1, and clear the coordinates to 0
    arr_locator[4].parent = arr_locator[0]
    arr_locator[4].translateX = 0
    arr_locator[4].translateY = 0
    arr_locator[4].translateZ = 0
    arr_locator[4].rotateX = 0
    arr_locator[4].rotateY = 0
    arr_locator[4].rotateZ = 0
    # Disconnect the parent-child relationship between locator 1 and locator 5
    arr_locator[4].parent = None
    # Let locator 5 be constrained under locator 4 and bake locator 5
    list = [(arr_locator[3], arr_locator[4])]
    with constraint_mg(list, True) as f2:
        back_anim(arr_locator[4])
    # Break the link between animations and objects
    for item in arr_Circle:
        item.delete_connection('%s.rx' % item)
        item.delete_connection('%s.ry' % item)
        item.delete_connection('%s.rz' % item)
        item.delete_connection('%s.tx' % item)
        item.delete_connection('%s.ty' % item)
        item.delete_connection('%s.tz' % item)
        item.delete_connection('%s.sx' % item)
        item.delete_connection('%s.sy' % item)
        item.delete_connection('%s.sz' % item)
        item.delete_connection('%s.v' % item)
    # Constrain the object to locator 5 and bake the object
    list1 = [(arr_locator[4], arr_Circle[0])]
    with constraint_mg(list1, True)as f3:
        back_anim(arr_Circle[0])
    #Delete the locator
    arr_locator[0].delete_node()
    arr_locator[4].delete_node()
   

  



