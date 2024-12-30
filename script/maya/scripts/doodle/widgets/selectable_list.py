from doodle import DoodleScrollArea


class SelectableList(DoodleScrollArea):
    def __init__(self, parent=None):
        super(SelectableList, self).__init__(parent=parent)
        self._selected_item = None
        self.items = {}

    def add_item(self, item):
        item.on_clicked.connect(self.on_clicked_item)
        self.add_widget(item)
        self.items[item.label] = item

    @property
    def current_item(self):
        return self._selected_item

    @current_item.setter
    def current_item(self, item):
        self._selected_item = item

    def set_current_item(self, index=None, name=None):
        if index is not None and len(self.items.values()) > 0:
            self._selected_item = self.items.values()[index]
            print self._selected_item
        elif name:
            self._selected_item = self.items[name]
        if self._selected_item is not None:
            self._selected_item.set_selection(True)

    def on_clicked_item(self, item):
        if self._selected_item is not None:
            self._selected_item.set_selection(False)
        self._selected_item = item
        self._selected_item.set_selection(True)

    def remove_item(self, item):
        self.items.pop(item.label)

    def clear(self):
        if self.items:
            self._selected_item = None
            items = self.items.values()
            self.items.clear()
            for item in items:
                item.deleteLater()
