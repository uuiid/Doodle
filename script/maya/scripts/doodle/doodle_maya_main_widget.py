# -*- coding: utf-8 -*-
import sys

from PySide2.QtCore import Qt, QSize
from PySide2.QtWidgets import QWidget, QApplication, QPushButton, QHBoxLayout, QSplitter, QStackedWidget

from doodle.doodle_maya import State
from doodle.widgets.analyze_references_widget import AnalyzeReferencesWidget
from doodle.widgets.sidebar import SidebarWidget
from doodle.widgets.create_cloth_widget import CreateClothWidget


class MainWidget(QWidget):
    def __init__(self, parent=None):
        super(MainWidget, self).__init__(parent)
        self.modules = {}
        self.setWindowFlags(self.windowFlags() | Qt.Window)
        self.setAttribute(Qt.WA_StyledBackground, True)
        self.layout = QHBoxLayout()
        # self.layout.setContentsMargins(10, 10, 10, 10)
        self.setLayout(self.layout)
        self.resize(QSize(800, 600))
        self.setObjectName('MainWidget')
        self.stacked_widget = QStackedWidget(self)
        self.splitter = QSplitter()
        self.splitter.setChildrenCollapsible(False)
        self.splitter.setHandleWidth(4)
        self.splitter.setObjectName('main-splitter')
        self.test_button = QPushButton('Test')
        self.sidebar_widget = SidebarWidget()
        self.test_button.clicked.connect(self.test_function)
        self.splitter.addWidget(self.sidebar_widget)
        self.splitter.addWidget(self.stacked_widget)
        self.splitter.setStretchFactor(0, 1)
        self.splitter.setStretchFactor(1, 2)
        self.layout.addWidget(self.splitter)
        self.state = State()
        self.state.sidebar_changed.connect(self.switch_module)
        self.init_modules()
        # self.setStyleSheet(main_style)
        self.setStyleSheet("""
        QPushButton {
            border-radius: 5px;
            background-color: #5d5d5d;
            padding: 5px;
            cursor: pointer;
        }
        
        QPushButton:hover {
            background-color: #596d7a;
        }
        
        QLabel {
            padding-left: 10px;
            padding-right: 10px;
        }
        """)
        # print self.styleSheet()

    def test_function(self, e):
        pass

    def test_function2(self, e):
        pass

    def init_modules(self):
        create_cloth = CreateClothWidget()
        self.add_module(name=u'创建布料', widget=create_cloth)
        empty = AnalyzeReferencesWidget()
        self.add_module(name=u'解析引用', widget=empty)
        self.state.sidebar = u'创建布料'

    def add_module(self, *args, **kwargs):
        if args:
            name = args[0]
            widget = args[1]
        else:
            name = kwargs.get('name')
            widget = kwargs.get('widget')
        self.modules[name] = widget
        self.sidebar_widget.add_item({'name': name})
        self.stacked_widget.addWidget(widget)

    def switch_module(self, name):
        self.stacked_widget.setCurrentWidget(self.modules[name])

    def closeEvent(self, e):
        super(MainWidget, self).closeEvent(e)


if __name__ == '__main__':
    APP = QApplication(sys.argv)
    gui = MainWidget()
    gui.show()
    APP.exec_()
