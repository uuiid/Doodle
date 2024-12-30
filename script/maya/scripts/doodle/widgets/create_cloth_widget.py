# -*- coding: utf-8 -*-

from PySide2.QtCore import Qt
from PySide2.QtWidgets import QWidget, QPushButton, QVBoxLayout, QSplitter, QMessageBox

from doodle import NameButtonWidget
from doodle.cell.h_box_layout_widget import HBoxLayoutWidget, HBoxLayoutLeftWidget, HBoxLayoutRightWidget
from doodle.doodle_maya import State
from doodle.style import main_style

from doodle.widgets.create_cloth_model_item import CreateClothModelItem
from doodle.widgets.doodle_scroll_area import DoodleScrollArea
import maya.cmds as cmds


class CreateClothWidget(QWidget):
    def __init__(self, parent=None):
        super(CreateClothWidget, self).__init__(parent)
        self.setAttribute(Qt.WA_StyledBackground, True)
        self.layout = QVBoxLayout()
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.main_widget = QWidget(self)
        self.main_layout = QVBoxLayout()
        self.main_layout.setContentsMargins(0, 0, 0, 0)
        self.main_widget.setObjectName('content_widget')
        self.main_widget.setLayout(self.main_layout)
        self.setLayout(self.layout)
        self.content_header = HBoxLayoutWidget()
        self.content_header_left_widget = HBoxLayoutLeftWidget()
        self.add_low_model_button = QPushButton(u"添加简模")
        self.add_collider_button = QPushButton(u"添加碰撞体")
        self.add_low_model_button.setCursor(Qt.PointingHandCursor)
        self.add_collider_button.setCursor(Qt.PointingHandCursor)
        self.content_header_left_widget.layout.addWidget(self.add_low_model_button)
        self.content_header_left_widget.layout.addWidget(self.add_collider_button)
        self.content_header_right_widget = HBoxLayoutRightWidget()
        self.content_header.layout.addWidget(self.content_header_left_widget)
        self.content_header.layout.addWidget(self.content_header_right_widget)
        self.content_footer = HBoxLayoutRightWidget()
        self.content_splitter = QSplitter(Qt.Horizontal)
        self.content_splitter.setHandleWidth(1)
        self.content_widget_left = DoodleScrollArea(self)
        self.content_widget_left.is_line = True
        self.content_widget_right = DoodleScrollArea(self)
        self.confirm_button = QPushButton(u"创建")
        self.confirm_button.setCursor(Qt.PointingHandCursor)
        self.content_footer.layout.addWidget(self.confirm_button)
        self.content_splitter.addWidget(self.content_widget_left)
        self.content_splitter.addWidget(self.content_widget_right)
        self.content_splitter.setStretchFactor(0, 3)
        self.content_splitter.setStretchFactor(1, 1)
        self.main_layout.addWidget(self.content_header)
        self.main_layout.addWidget(self.content_splitter)
        self.main_layout.addWidget(self.content_footer)
        self.main_layout.setStretch(0, 0)
        self.main_layout.setStretch(1, 1)
        self.main_layout.setStretch(2, 0)
        self.layout.addWidget(self.main_widget)
        self.content_widget_left_item = []
        self.content_widget_right_item = []
        self.state = State()
        self.confirm_button.clicked.connect(self.on_confirm_button_clicked)
        self.add_low_model_button.clicked.connect(self.add_cloth_model_item)
        self.add_collider_button.clicked.connect(self.add_collider_item)
        # self.setStyleSheet(main_style)

    def init(self):
        for i in range(10):
            self.add_cloth_model_item(i)

    def add_cloth_model_item(self, *args):
        names = self.state.check_low_model()
        if names:
            for name in names:
                self.state.low_model[name] = []
                item = CreateClothModelItem()
                item.label = name
                item.name_label.setText(name)
                self.content_widget_left_item.append(item)
                self.state.low_model[name] = []
                item.on_remove.connect(self.remove_item)
                # item.setObjectName('sidebar-item')
                self.content_widget_left.add_widget(item)

    def add_collider_item(self, *args):
        names = self.state.check_collide()
        if names:
            for name in names:
                self.state.collide.append(name)
                item = NameButtonWidget()
                item.label = name
                item.name.setText(name)
                item.button.setText('-')
                item.on_remove.connect(self.remove_collider_item)
                self.content_widget_right_item.append(item)
                self.content_widget_right.add_widget(item)

    def on_confirm_button_clicked(self):
        self.state.confirm_data()
        self.state.low_model = {}
        self.state.collide = []
        self.clear_items()

    def clear_items(self):
        for i in self.content_widget_left_item:
            i.deleteLater()
        for i in self.content_widget_right_item:
            i.deleteLater()

    def remove_item(self, item):
        self.state.low_model.pop(item.label)
        item.deleteLater()

    def remove_collider_item(self, name):
        self.state.collide.remove(name)
