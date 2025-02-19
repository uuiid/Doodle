# -*- coding: utf-8 -*-
from PySide2.QtCore import Signal, QSize, Qt
from PySide2.QtWidgets import QLabel, QPushButton, QHBoxLayout
from doodle.cell.h_box_layout_widget import HBoxLayoutWidget, HBoxLayoutRightWidget, HBoxLayoutLeftWidget
from doodle.cell.v_box_layout_widget import VBoxLayoutWidget
from doodle.doodle_maya import State, StateSignal
from doodle.widgets.high_model_widget import HighModelWidget


class CreateClothModelItemHeader(HBoxLayoutWidget):
    def __init__(self, parent=None):
        super(CreateClothModelItemHeader, self).__init__(parent=parent)
        self.setObjectName('header')
        self.setAttribute(Qt.WA_StyledBackground, True)
        self.setStyleSheet("""
            background-color: #373737;
        """)


class CreateClothModelItem(VBoxLayoutWidget):
    on_remove = Signal(object)

    def __init__(self, parent=None):
        super(CreateClothModelItem, self).__init__(parent)
        self.header = CreateClothModelItemHeader()
        self.setAttribute(Qt.WA_StyledBackground, True)
        self.header.setFixedHeight(22)
        self.header_left = HBoxLayoutLeftWidget()
        self.header_title = QLabel()
        self.header_title.setCursor(Qt.PointingHandCursor)
        self.header_title.mousePressEvent = self.on_mouse_press
        self.open_button = QPushButton()
        self.open_button.setFixedWidth(20)
        self.open_button.setText('+')
        self.open_button.setCursor(Qt.PointingHandCursor)
        self.close_button = QPushButton()
        self.close_button.setFixedWidth(20)
        self.close_button.setText('-')
        self.close_button.setCursor(Qt.PointingHandCursor)
        self.remove_button = QPushButton(u"移除")
        self.remove_button.setMaximumWidth(80)
        self.remove_button.setCursor(Qt.PointingHandCursor)
        self.remove_widget = HBoxLayoutRightWidget()
        self.remove_widget.layout.addWidget(self.remove_button)
        self.header_left.layout.addWidget(self.open_button)
        self.header_left.layout.addWidget(self.close_button)
        self.header_left.layout.addWidget(self.header_title)
        self.header.layout.addWidget(self.header_left)
        self.header.layout.addWidget(self.remove_widget)
        self.name_label = QLabel("")
        self.name_label.hide()
        self._label = ''
        # self.mian_widget.setObjectName('create-cloth-model-item')
        self.content_widget = HBoxLayoutWidget(self)
        self.high_model_widget = HighModelWidget(self)
        self.content_widget.layout.addWidget(self.name_label)
        self.content_widget.layout.addWidget(self.high_model_widget)
        self.content_widget.layout.setSpacing(2)
        self.content_widget.layout.setStretch(0, 1)
        self.content_widget.layout.setStretch(1, 2)
        self.layout.addWidget(self.header)
        self.layout.addWidget(self.content_widget)
        self.setMaximumHeight(200)
        self.setMinimumHeight(200)
        self.open_button.hide()
        self.open_button.clicked.connect(self.on_click_open)
        self.close_button.clicked.connect(self.on_click_close)
        self.remove_button.clicked.connect(self.on_remove_button_clicked)
        # self.setStyleSheet(main_style)

    def on_remove_button_clicked(self, *args):
        self.on_remove.emit(self)

    @property
    def label(self):
        return self._label

    @label.setter
    def label(self, value):
        self._label = value
        value = value.split("|")[-1]
        self.header_title.setText(value)

    def on_click_close(self):
        self.open_button.show()
        self.close_button.hide()
        self.content_widget.hide()
        self.setMaximumHeight(22)
        self.setMinimumHeight(22)

    def on_click_open(self):
        self.close_button.show()
        self.open_button.hide()
        self.content_widget.show()
        self.setMaximumHeight(200)
        self.setMinimumHeight(200)

    def on_mouse_press(self, e):
        State.select_maya_object([self._label])
