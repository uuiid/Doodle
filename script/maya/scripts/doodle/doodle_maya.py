# -*- coding: utf-8 -*-
import threading
import inspect
from collections import OrderedDict

from PySide2.QtWidgets import QMessageBox, QWidget
from maya import cmds, mel
from shiboken2 import wrapInstance


class StateSignal(object):
    def __init__(self):
        super(StateSignal, self).__init__()
        self._func = []

    def emit(self, *args):
        for i in self._func:
            if callable(i):
                if inspect.getargspec(i).varargs:
                    i(*args)
                elif inspect.getargspec(i).args[0] == 'self':
                    args_l = len(inspect.getargspec(i).args)
                    i(*(args[0:args_l - 1]))
                else:
                    args_l = len(inspect.getargspec(i).args)
                    i(*(args[0:args_l]))

    def connect(self, func):
        self._func.append(func)


class State(object):
    _instance = None
    _lock = threading.Lock()
    sidebar_changed = StateSignal()

    def __new__(cls, *args, **kwargs):
        with cls._lock:
            if cls._instance is None:
                cls._instance = super(State, cls).__new__(cls)
        return cls._instance

    def __init__(self):
        if hasattr(self, '_initialized') and self._initialized:
            return
        super(State, self).__init__()
        if not hasattr(self, '_initialized'):
            self._initialized = True
            self._sidebar = ''
            self.low_model = OrderedDict()
            self.collide = []
            self.doodle_tool_widget = None
            # self.high_model = {}

    @property
    def sidebar(self):
        return self._sidebar

    @sidebar.setter
    def sidebar(self, value):
        old_value = self.sidebar
        self._sidebar = value
        self.sidebar_changed.emit(value, old_value)

    def check_low_model(self):
        try:
            selected_objects = State.is_polygon_model_selected()
            objects = selected_objects
            if selected_objects:
                for obj in selected_objects:
                    if obj in self.low_model:
                        objects.remove(obj)
                        cmds.warning(u"已存在" + obj)

                return objects
        except EOFError:
            return False

    def check_high_model(self, low_model):
        try:
            selected_objects = State.is_polygon_model_selected()
            objects = selected_objects
            if selected_objects:
                for obj in selected_objects:
                    if obj in self.low_model.get(low_model):
                        objects.remove(obj)
                        cmds.warning(u"已存在" + obj)
                        # QMessageBox().warning(None, 'Warning', u"已存在" + selected_objects[0])

                return objects
        except Exception as e:
            return False

    def check_collide(self):
        selected_objects = State.is_polygon_model_selected()
        objects = selected_objects
        if selected_objects:
            for obj in selected_objects:
                if obj in self.collide:
                    objects.remove(obj)
                    cmds.warning(u"已存在" + selected_objects[0])
                    # QMessageBox().warning(None, 'Warning', u"已存在" + selected_objects[0])

            # return False
            return objects

    def confirm_data(self):
        cloth = self.format_data()
        print cloth
        if cloth:
            cmds.doodle_create_qcloth_assets(cloth=cloth, collision=self.collide)

    def format_data(self):
        cloth = []
        for key in self.low_model.keys():
            value = self.low_model.get(key)
            if value:
                high_model = key
                for i in value:
                    high_model += ';' + i
                cloth.append(high_model)
        return cloth



    @staticmethod
    def get_maya_selection():
        selected_objects = cmds.ls(selection=True, long=True)
        if not selected_objects:
            cmds.warning(u"请先选择一个模型")
            return []
        return selected_objects

    @staticmethod
    def is_polygon_model_selected():
        selected_objects = State.get_maya_selection()
        if selected_objects:
            objects = selected_objects
            for obj in selected_objects:
                # 获取对象下的子节点
                shapes = cmds.listRelatives(obj, shapes=True, fullPath=True) or []
                for shape in shapes:
                    # 检查子节点是否是多边形网格
                    if cmds.objectType(shape) != "mesh":
                        objects.remove(shape)
                return objects
            else:
                cmds.warning(u"请选择多边形模型")
                QMessageBox().warning(None, 'Warning', u"请选择多边形模型", None, None)
        return []

    @staticmethod
    def select_maya_object(object_names):
        cmds.select(object_names)

    @staticmethod
    def analyze_references():
        if len(State.get_analyze_references_nodes()) > 0:
            return cmds.doodle_file_info_edit(f=True)
        else:
            return cmds.doodle_file_info_edit()

    @staticmethod
    def get_analyze_references_nodes():
        return cmds.ls(type="doodle_file_info", long=True)

    @staticmethod
    def set_doodle_file_info_attr(attr, value):
        cmds.setAttr(attr, value)

    @staticmethod
    def get_doodle_file_info_attr(attr):
        try:
            if cmds.listConnections(attr):
                return cmds.listConnections(attr)[0]
            return cmds.getAttr(attr)
        except EOFError:
            return None

    @staticmethod
    def connect_wind_field(attrs):
        selected_objects = cmds.ls(selection=True, long=True)
        if len(selected_objects) > 0:
            if attrs[0]:
                connections = cmds.listConnections(attrs[1] + '.wind_field')
                if connections:
                    cmds.disconnectAttr(attrs[0] + '.message', attrs[1] + '.wind_field')
            cmds.connectAttr(selected_objects[0] + '.message', attrs[1] + ".wind_field")
            return selected_objects[0]

    @staticmethod
    def get_wind_field(attr):
        return cmds.listConnections(attr)

    @staticmethod
    def get_connect_collide(parent_attr):
        return cmds.listConnections(parent_attr + '.collision_objects')

    @staticmethod
    def connect_collide(parent_attr):
        connections = State.get_connect_collide(parent_attr)
        connections_num = 0
        if connections:
            for connection in connections:
                if not connection.startswith('|'):
                    connection = "|" + connection
                    connections[connections_num] = connection
                connections_num += 1
        else:
            connections = []
        selected_objects = State.is_polygon_model_selected()
        return_object = {}
        if len(selected_objects) > 0:
            for obj in selected_objects:
                if obj not in connections:
                    cmds.connectAttr(obj + '.message',
                                     parent_attr + ".collision_objects[{}]".format(str(connections_num)))
                    return_object[obj] = str(connections_num)
                    connections_num += 1
                else:
                    cmds.warning(u"已存在" + obj)
        return return_object

    @staticmethod
    def disconnect_collide(attrs):
        collides = State.get_connect_collide(attrs[1])
        if collides:
            attr = attrs[1] + ".collision_objects[{}]".format(attrs[0])
            cmds.disconnectAttr(collides[int(attrs[0])] + '.message', attr)

    @staticmethod
    def get_maya_main_window(omui=None):
        main_window_ptr = omui.MQtUtil.mainWindow()
        return wrapInstance(long(main_window_ptr), QWidget)

    @staticmethod
    def add_menu():
        if cmds.window('MayaWindow', exists=True):
            main_maya_window = mel.eval('$temp1=$gMainWindow')
            menu_name = "doodle"
            if not cmds.menu(menu_name, exists=True):
                cmds.menu(menu_name, label="doodle", parent=main_maya_window)
                cmds.menuItem(label="doodle_tool", parent=menu_name, command=my_custom_function)

        else:
            print "Maya main window does not exist."
    @staticmethod
    def remove_menu():
        cmds.deleteUI("doodle", menu=True)