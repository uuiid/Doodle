
from PySide2.QtWidgets import QWidget, QHBoxLayout, QLabel

from doodle.cell.selectable_item import SelectableItem
from doodle.widgets.doodle_scroll_area import DoodleScrollArea
from doodle.doodle_maya import State


class SidBarWidgetItem(SelectableItem):
    def __init__(self, parent=None):
        super(SidBarWidgetItem, self).__init__(parent)
        self.state = State()

    def mousePressEvent(self, event):
        super(SidBarWidgetItem, self).mousePressEvent(event)
        self.state.sidebar = self.name.text()




class SidebarWidget(QWidget):
    def __init__(self, parent=None, data=None):
        super(SidebarWidget, self).__init__(parent)
        self.items = {}
        self.layout = QHBoxLayout()
        self.setLayout(self.layout)
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.data = data
        self.area = DoodleScrollArea(self)
        self.layout.addWidget(self.area)
        self.state = State()
        self.state.sidebar_changed.connect(self.on_sidebar_changed)
        # self.state.sidebar.sidebar_changed.connect(self.on_sidebar_changed)

    def __init_widget(self):
        if self.data:
            for i in self.data:
                self.add_item(i)

    def add_item(self, item):
        _item = SidBarWidgetItem()
        _item.name.setText(item['name'])
        self.items[item['name']] = _item
        self.area.add_widget(_item)

    def on_sidebar_changed(self, new_value, old_value):
        item = self.items.get(new_value)
        item.set_selection(True)
        old_item = self.items.get(old_value)
        if old_item:
            old_item.set_selection(False)

    def mousePressEvent(self, event):
        super(SidebarWidget, self).mousePressEvent(event)
