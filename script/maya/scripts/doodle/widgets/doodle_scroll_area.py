# -*- coding: utf-8 -*-

from PySide2.QtCore import Qt
from PySide2.QtWidgets import QScrollArea, QVBoxLayout, QSpacerItem, QSizePolicy, QWidget, QPushButton, QLabel, QFrame

from doodle.style import main_style


class DoodleScrollArea(QWidget):
    def __init__(self, parent=None):
        super(DoodleScrollArea, self).__init__(parent)
        self.setAttribute(Qt.WA_StyledBackground, True)
        # 创建滚动区域
        self.scroll_area = QScrollArea(self)
        self.scroll_area.setWidgetResizable(True)
        self.content_widget = QWidget(self)
        self.content_layout = QVBoxLayout(self.content_widget)
        self.content_widget.setObjectName("doodle-scroll-area")
        self.content_widget.setStyleSheet("""
        #doodle-scroll-area {
        border:1px solid rgb(0,0,0,1);
        border-radius:10px;
        background-color: transparent;
            }
        """)
        self.content_widget.setLayout(self.content_layout)
        # 创建内容部件和布局
        self.scroll_content = QWidget()
        self.scroll_content.setObjectName('scroll-area')
        self.scroll_layout = QVBoxLayout(self.scroll_content)  # 垂直布局
        self.scroll_layout.setContentsMargins(0, 0, 0, 0)
        self.scroll_layout.addItem(QSpacerItem(0, 0, QSizePolicy.Expanding, QSizePolicy.Expanding))
        self.scroll_area.setWidget(self.scroll_content)  # 设置内容部件
        self.content_layout.addWidget(self.scroll_area)
        self._is_line = False
        # 创建按钮，用于动态添加子组件
        # self.add_button = QPushButton("添加子组件")
        # self.add_button.clicked.connect(self.add_widget)

        # 将按钮和滚动区域添加到主布局
        self.layout = QVBoxLayout()
        self.layout.setContentsMargins(0, 0, 0, 0)
        # self.main_layout.addWidget(self.add_button)
        self.layout.addWidget(self.content_widget)

        # 设置主窗口的中心部件
        self.setLayout(self.layout)
        #self.setStyleSheet(main_style)

        # 计数器，用于跟踪添加的子组件数量
        self.label_counter = 0

    @property
    def is_line(self):
        return self._is_line

    @is_line.setter
    def is_line(self, value):
        self._is_line = value
        # if value and self.scroll_layout.count() > 1:
        #     for i in range(1, self.scroll_layout.count() / 2, 2):
        #         line = QFrame()
        #         line.setFrameShape(QFrame.HLine)
        #         line.setFrameShadow(QFrame.Sunken)
        #         self.scroll_layout.insertWidget(i, line)

    def add_widget(self, widget):
        """动态添加子组件"""
        # widget.setAttribute(Qt.WA_TransparentForMouseEvents)
        # if self.label_counter >= 1 and self._is_line:
        #     line = QFrame()
        #     line.setStyleSheet('background-color: rgba(0, 0, 0,0.5);')
        #     line.setMaximumHeight(1)
        #     line.setFrameShape(QFrame.HLine)
        #     line.setFrameShadow(QFrame.Sunken)
        #     self.scroll_layout.insertWidget(self.label_counter, line)
        #     self.label_counter += 1
        self.scroll_layout.insertWidget(len(self.scroll_content.children())-1, widget)  # 添加到布局
        self.scroll_content.adjustSize()  # 调整内容部件的大小
        self.label_counter += 1

    def mousePressEvent(self, event):
        super(DoodleScrollArea, self).mousePressEvent(event)
