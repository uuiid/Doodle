# -*- coding: utf-8 -*-
from PySide2.QtCore import Qt
from PySide2.QtWidgets import QPushButton, QSplitter, QStackedWidget, QLabel

from doodle import VBoxLayoutWidget, HBoxLayoutWidget
from doodle.cell.h_box_layout_widget import HBoxLayoutLeftWidget, HBoxLayoutCenterWidget
from doodle.cell.selectable_item import SelectableItem
from doodle.doodle_maya import State
from doodle.widgets.analyze_references_content_widget import AnalyzeReferencesContent
from doodle.widgets.selectable_list import SelectableList


class AnalyzeReferencesWidgetItem(SelectableItem):
    def __init__(self, parent):
        super(AnalyzeReferencesWidgetItem, self).__init__(parent)
        self.state = State()


class AnalyzeReferencesWidget(VBoxLayoutWidget):
    def __init__(self):
        super(AnalyzeReferencesWidget, self).__init__()
        self.prompt_widget = HBoxLayoutCenterWidget()
        self.prompt_label = QLabel(u"当前未解析引用")
        self.prompt_widget.layout.addWidget(self.prompt_label)
        self.prompt_widget.hide()
        self.content_header = HBoxLayoutWidget()
        self.content_header.layout.setContentsMargins(10,0,0,0)
        self.content_header.setFixedHeight(30)
        self.content_header_left = HBoxLayoutLeftWidget()
        self.analyze_references_button = QPushButton(u"解析引用")
        self.content_header_left.layout.addWidget(self.analyze_references_button)
        self.content_header.layout.addWidget(self.content_header_left)
        self.content_splitter = QSplitter(Qt.Horizontal)
        self.content_left = SelectableList()
        self.content_left.setMaximumWidth(200)
        self.content_right = QStackedWidget(self)
        self.content_splitter.addWidget(self.content_left)
        self.content_splitter.addWidget(self.content_right)
        self.content_splitter.setStretchFactor(0, 1)
        self.content_splitter.setStretchFactor(1, 3)
        self.layout.addWidget(self.content_header)
        self.layout.addWidget(self.content_splitter)
        self.layout.addWidget(self.prompt_widget)
        self.analyze_references_button.clicked.connect(self.analyze_references_button_clicked)
        self.all_stack_widget = {}
        self.is_initialized = False
        self.state = State()
        self.state.sidebar_changed.connect(self.on_initialize)

    def initialize(self):
        nodes = State.get_analyze_references_nodes()
        # nodes = [{'name': "test", 'data': 2.550}, {'name': "test2", 'data': 1.222}]
        for node in nodes:
            if node not in self.all_stack_widget.keys():
                item = SelectableItem()
                left_widget = AnalyzeReferencesContent(label=node)
                left_widget.header_title.setText(node)
                item.label = node
                self.content_right.addWidget(left_widget)
                self.all_stack_widget[node] = left_widget
                item.on_clicked.connect(self.toggle_widget)
                item.name.setText(node)
                self.content_left.add_item(item)
        if self.content_left.current_item is None:
            self.content_left.set_current_item(0)
        
        if self.all_stack_widget != {}:
            self.content_splitter.show()
            self.prompt_widget.hide()
            self.analyze_references_button.setText(u"重新解析引用")
            self.is_initialized = True
        else:
            self.content_splitter.hide()
            self.prompt_widget.show()
            self.analyze_references_button.setText(u"解析引用")

    def analyze_references_button_clicked(self):
        State.analyze_references()
        self.refresh()

    def toggle_widget(self, name):
        if name:
            self.content_right.setCurrentWidget(self.all_stack_widget[name.label])

    def on_initialize(self, *args):
        if args[0] == u'解析引用':
            if not self.is_initialized:
                self.initialize()


    def refresh(self):
        self.clear_all()
        self.initialize()

    def clear_all(self):
        if self.all_stack_widget != {}:
            for node in self.all_stack_widget.keys():
                self.content_right.removeWidget(self.all_stack_widget[node])
                self.all_stack_widget[node].deleteLater()
            self.all_stack_widget = {}
        self.content_left.clear()
