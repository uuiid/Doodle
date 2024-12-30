# -*- coding: utf-8 -*-

from PySide2.QtWidgets import QPushButton

from doodle.cell.v_box_layout_widget import VBoxLayoutWidget
from doodle.cell.name_button_widget import NameButtonWidget
from doodle.cell.h_box_layout_widget import HBoxLayoutCenterWidget
from doodle.widgets.doodle_scroll_area import DoodleScrollArea
from doodle.doodle_maya import State


class HighModelWidget(VBoxLayoutWidget):
    def __init__(self, parent=None):
        super(HighModelWidget, self).__init__(parent)
        self.parent = parent
        self.scroll_area = DoodleScrollArea(self)
        self.scroll_area.content_widget.setObjectName('doodle-scroll-area-t')
        self.scroll_area.is_line = True
        self.scroll_area.setMinimumHeight(80)
        self.layout.addWidget(self.scroll_area)
        self.add_high_modulus_button = QPushButton(u"添加高模")
        self.add_high_modulus_widget = HBoxLayoutCenterWidget()
        self.add_high_modulus_widget.layout.addWidget(self.add_high_modulus_button)
        self.add_high_modulus_button.clicked.connect(self.add_high_model)
        self.layout.addWidget(self.add_high_modulus_widget)
        self.state = State()

    def add_high_model(self):
        names = self.state.check_high_model(self.parent.label)
        print names
        if names:
            for name in names:
                item = NameButtonWidget()
                item.mian_widget.setObjectName('high-model-widget-item')
                item.button.setText('-')
                item.name.setText(name)
                item.label = name
                item.on_remove.connect(self.remove_high_model)
                item.on_clicked.connect(self.on_clicked_high_model)
                self.state.low_model[self.parent.label].append(name)
                self.scroll_area.add_widget(item)

    def remove_high_modulus(self, name):
        self.state.low_model[self.parent.label].remove(name)

    @staticmethod
    def on_clicked_high_model(name):
        State.select_maya_object(name)
