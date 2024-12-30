# -*- coding: utf-8 -*-

from PySide2.QtCore import Qt
from PySide2.QtWidgets import QSplitter, QPushButton, QCheckBox, QLabel, QLineEdit, QSlider, \
    QDoubleSpinBox, QAbstractSpinBox, QSizePolicy, QSpacerItem

from doodle import HBoxLayoutWidget, DoodleScrollArea, VBoxLayoutWidget, NameButtonWidget
from doodle.cell.h_box_layout_widget import HBoxLayoutRightWidget, HBoxLayoutLeftWidget, HBoxLayoutCenterWidget
from doodle.doodle_maya import State


class DoubleSpinBox(QDoubleSpinBox):
    def __init__(self, parent=None):
        super(DoubleSpinBox, self).__init__(parent)
        self.setDecimals(3)
        self.setButtonSymbols(QAbstractSpinBox.NoButtons)


class ImportableLabelBaseWidget(HBoxLayoutWidget):

    def __init__(self, parent=None):
        super(ImportableLabelBaseWidget, self).__init__(parent)
        self.parent = parent
        self.name = QLabel()
        self.name_widget = HBoxLayoutRightWidget()
        self.name_widget.layout.addWidget(self.name)
        self.name_widget.setFixedWidth(200)
        self.layout.addWidget(self.name_widget)
        self._label = ''

    def initialize(self):
        pass

    def emit_value(self):
        pass

    def set_value(self, value):
        pass

    @property
    def label(self):
        return self._label

    @label.setter
    def label(self, value):
        self._label = value
        self.initialize()


class ImportableLabelWidget(ImportableLabelBaseWidget):
    def __init__(self, parent=None):
        super(ImportableLabelWidget, self).__init__(parent)
        self.input = QLineEdit()
        self.layout.addWidget(self.input)
        self.input.textChanged.connect(self.input_value_changed)

    def initialize(self):
        if self.parent and self._label:
            attr = self.parent.label + '.' + self._label
            _value = State.get_doodle_file_info_attr(attr)
            if _value:
                self.set_value(_value)

    def emit_value(self):
        try:
            State.set_doodle_file_info_attr(self.parent.label + '.' + self._label, self.input.text())
        except AttributeError:
            pass

    def input_value_changed(self, value):
        pass

    def set_value(self, value):
        self.input.setText(value)


class SliderLabelWidget(ImportableLabelBaseWidget):
    def __init__(self, parent=None):
        super(SliderLabelWidget, self).__init__(parent)
        self._multiple = 1000
        self.input = DoubleSpinBox()
        self.input.setMaximum(10.000)
        self.layout.addWidget(self.input)
        self.input.setFixedWidth(60)
        self.slider = QSlider(Qt.Horizontal)
        self.slider.setMinimum(0)
        self.slider.setMaximum(10000)
        self.layout.addWidget(self.slider)
        self.slider.valueChanged.connect(self.slider_value_changed)
        self.input.valueChanged.connect(self.input_value_changed)

    @property
    def multiple(self):
        return self._multiple

    @multiple.setter
    def multiple(self, value):
        self._multiple = 100 * value
        self.input.setMaximum(value)
        self.slider.setMaximum(value * self._multiple)

    def initialize(self):
        if self.parent and self._label:
            attr = self.parent.label + '.' + self._label
            _value = State.get_doodle_file_info_attr(attr)
            if _value:
                self.set_value(_value)

    def emit_value(self):
        try:
            State.set_doodle_file_info_attr(self.parent.label + '.' + self._label, self.input.value())
        except AttributeError:
            pass

    def set_minimum(self, value):
        self.input.setMinimum(value)
        self.slider.setMinimum(value)

    def set_maximum(self, value):
        self.input.setMaximum(value)
        self._multiple = 100 * value
        self.slider.setMaximum(value * self._multiple)

    def input_value_changed(self, value):
        # if self.input.value() and self.slider.value() != self.input.value():
        self.slider.setValue(self.input.value() * self._multiple)

    def slider_value_changed(self, value):
        self.input.setValue(value / float(self._multiple))
        self.emit_value()

    def set_value(self, value):
        self.slider.setValue(value * self._multiple)
        self.input.setValue(value)


class SelectableLabelWidget(ImportableLabelWidget):
    def __init__(self, parent=None):
        super(SelectableLabelWidget, self).__init__(parent)
        self.input.setReadOnly(True)
        self.select_button = QPushButton()
        self.select_button.setText("Select")
        self.select_button.clicked.connect(self.set_input)
        self.layout.addWidget(self.select_button)

    def initialize(self):
        if self.parent and self._label:
            connections = State.get_wind_field(self.parent.label + '.' + self._label)
            if connections:
                self.set_value(connections[0])

    def set_input(self):
        self.input.setText(State.connect_wind_field([self.input.text(), self.parent.label]))


class MultiInputLabelWidget(ImportableLabelBaseWidget):
    def __init__(self, parent=None):
        super(MultiInputLabelWidget, self).__init__(parent)
        self.input_widget = HBoxLayoutLeftWidget()
        self.input_x = DoubleSpinBox()
        self.input_y = DoubleSpinBox()
        self.input_z = DoubleSpinBox()
        self.input_x.valueChanged.connect(self.emit_value)
        self.input_y.valueChanged.connect(self.emit_value)
        self.input_z.valueChanged.connect(self.emit_value)
        self.input_x.setMaximum(100000000)
        self.input_y.setMaximum(100000000)
        self.input_z.setMaximum(100000000)
        self.input_x.setMinimum(-10000000)
        self.input_y.setMinimum(-10000000)
        self.input_z.setMinimum(-10000000)
        self.input_x.setFixedWidth(60)
        self.input_y.setFixedWidth(60)
        self.input_z.setFixedWidth(60)
        self.input_widget.layout.addWidget(self.input_x)
        self.input_widget.layout.addWidget(self.input_y)
        self.input_widget.layout.addWidget(self.input_z)
        self.main_layout.addWidget(self.input_widget)
        self.input_widget.layout.addItem(QSpacerItem(0, 0, QSizePolicy.Expanding, QSizePolicy.Minimum))

    def initialize(self):
        try:
            _value = {'x': 0, 'y': 0, 'z': 0}
            attr_name = self.parent.label + '.' + self._label.lower()
            _value['x'] = State.get_doodle_file_info_attr(attr_name + 'X')
            _value['y'] = State.get_doodle_file_info_attr(attr_name + 'Y')
            _value['z'] = State.get_doodle_file_info_attr(attr_name + 'Z')
            self.set_value(_value)
        except AttributeError:
            pass

    def set_value(self, value):
        self.input_x.setValue(value.get('x'))
        self.input_y.setValue(value.get('y'))
        self.input_z.setValue(value.get('z'))

    def emit_value(self):
        try:
            attr_name = self.parent.label + '.' + self._label.lower()
            State.set_doodle_file_info_attr(attr_name + 'X', self.input_x.value())
            State.set_doodle_file_info_attr(attr_name + 'Y', self.input_y.value())
            State.set_doodle_file_info_attr(attr_name + 'Z', self.input_z.value())
        except AttributeError:
            pass


class IntInputLabelWidget(SliderLabelWidget):
    def __init__(self, parent=None):
        super(IntInputLabelWidget, self).__init__(parent)
        self.input.setDecimals(0)


class DoodleCheckBoxWidget(QCheckBox):
    def __init__(self, parent=None):
        super(DoodleCheckBoxWidget, self).__init__(parent)
        self.parent = parent
        self._label = ''
        self.clicked.connect(self.state_changed)

    def initialize(self):
        try:
            check_state = State.get_doodle_file_info_attr(self.parent.label + '.' + self._label)
            self.setCheckState(Qt.Checked if check_state else Qt.Unchecked)
        except AttributeError:
            pass

    def state_changed(self):
        try:
            Qt.CheckState()
            State.set_doodle_file_info_attr(self.parent.label + '.' + self._label, self.isChecked())
        except AttributeError as e:
            print e

    @property
    def label(self):
        return self._label

    @label.setter
    def label(self, value):
        self._label = value
        self.initialize()


class AnalyzeReferencesContent(VBoxLayoutWidget):
    def __init__(self, parent=None, label=None):
        super(AnalyzeReferencesContent, self).__init__(parent=parent)
        self.header = HBoxLayoutWidget()
        self.label = label
        self.header.main_layout.setContentsMargins(10, 0, 0, 0)
        # self.header.layout.setContentsMargins(10, 0, 0, 0)
        self.header.main_widget.setStyleSheet("background-color: #373737")
        self.header.setFixedHeight(30)
        self.header_title_widget = HBoxLayoutWidget()
        self.header_title = QLabel()
        self.header_title_widget.layout.addWidget(self.header_title)
        self.header.layout.addWidget(self.header_title_widget)
        self.content_splitter = QSplitter(Qt.Horizontal)
        self.content_left = DoodleScrollArea()
        self.content_left.scroll_layout.setContentsMargins(10, 10, 10, 10)
        self.content_right = VBoxLayoutWidget()
        self.add_collide_header = HBoxLayoutLeftWidget()
        self.add_collide_button = QPushButton(u"添加碰撞体")
        self.add_collide_header.main_layout.setContentsMargins(10, 0, 0, 0)
        self.add_collide_header.layout.addWidget(self.add_collide_button)
        self.reference_file = ImportableLabelWidget(self)
        self.reference_file.name.setText('Reference File')
        self.reference_file.label = 'reference_file'
        self.reference_file.input.setReadOnly(True)
        self.reference_file_path = ImportableLabelWidget(self)
        self.reference_file_path.name.setText('Reference File Path')
        self.reference_file_path.input.setReadOnly(True)
        self.reference_file_path.label = 'reference_file_path'
        self.reference_file_namespace = ImportableLabelWidget(self)
        self.reference_file_namespace.name.setText('Reference File Namespace')
        self.reference_file_namespace.label = 'reference_file_namespace'
        self.reference_file_namespace.input.setReadOnly(True)
        self.wind_field = SelectableLabelWidget(self)
        self.wind_field.name.setText('Wind Field')
        self.wind_field.label = 'wind_field'
        self.frame_samples = IntInputLabelWidget(self)
        self.frame_samples.name.setText('Frame Samples')
        self.frame_samples.label = 'frame_samples'
        self.frame_samples.set_minimum(1)
        self.time_scale = SliderLabelWidget(self)
        self.time_scale.name.setText('Time Scale')
        self.time_scale.label = 'time_scale'
        self.length_scale = SliderLabelWidget(self)
        self.length_scale.name.setText('Length Scale')
        self.length_scale.label = 'length_scale'
        self.length_scale.multiple = 100
        self.max_cg_iteration = IntInputLabelWidget(self)
        self.max_cg_iteration.name.setText('Max CGIteration')
        self.max_cg_iteration.label = 'max_cg_iteration'
        self.max_cg_iteration.multiple = 1000
        self.cg_accuracy = IntInputLabelWidget(self)
        self.cg_accuracy.name.setText('CG Accuracy')
        self.cg_accuracy.label = 'cg_accuracy'
        self.cg_accuracy.set_maximum(9)
        self.gravity = MultiInputLabelWidget(self)
        self.gravity.name.setText('Gravity')
        self.gravity.label = 'gravity'
        self.collide_list = DoodleScrollArea()
        self.collide_list.setMinimumWidth(120)
        self.selectable_widget = HBoxLayoutCenterWidget()
        self.is_Solve = DoodleCheckBoxWidget(self)
        self.is_Solve.setText('Is Solve')
        self.is_Solve.label = 'is_solve'
        self.sim_override = DoodleCheckBoxWidget(self)
        self.sim_override.setText('Sim Override')
        self.sim_override.label = 'sim_override'
        self.simple_subsampling = DoodleCheckBoxWidget(self)
        self.simple_subsampling.setText('Simple Subsampling')
        self.simple_subsampling.label = 'simple_subsampling'
        self.content_left.add_widget(self.reference_file)
        self.content_left.add_widget(self.reference_file_path)
        self.content_left.add_widget(self.reference_file_namespace)
        self.content_left.add_widget(self.selectable_widget)
        self.content_left.add_widget(self.wind_field)
        self.content_left.add_widget(self.frame_samples)
        self.content_left.add_widget(self.time_scale)
        self.content_left.add_widget(self.length_scale)
        self.content_left.add_widget(self.max_cg_iteration)
        self.content_left.add_widget(self.cg_accuracy)
        self.content_left.add_widget(self.gravity)
        self.selectable_widget.layout.addWidget(self.is_Solve)
        self.selectable_widget.layout.addWidget(self.sim_override)
        self.selectable_widget.layout.addWidget(self.simple_subsampling)
        self.content_right.layout.addWidget(self.add_collide_header)
        self.content_right.layout.addWidget(self.collide_list)
        self.content_splitter.addWidget(self.content_left)
        self.content_splitter.addWidget(self.content_right)
        self.layout.addWidget(self.header)
        self.layout.addWidget(self.content_splitter)
        self.add_collide_button.clicked.connect(self.add_collide)
        self.collide_objects = []
        self.initialize()

    def initialize(self):
        if self.label:
            collides = State.get_connect_collide(self.label)
            if collides:
                num = 0
                for collide in collides:
                    self.add_collide_item(collide, str(num))
                    num += 1

    def add_collide(self):
        collides = State.connect_collide(self.label)
        for collide in collides.keys():
            self.add_collide_item(collide, collides[collide])

    def add_collide_item(self, collide, label):
        collide_item = NameButtonWidget()
        collide_item.button.setText('-')
        collide_item.name.setText(collide)
        collide_item.label = {'name': collide, 'label': label}
        collide_item.on_remove.connect(self.remove_collide)
        self.collide_objects.append(collide)
        self.collide_list.add_widget(collide_item)

    def remove_collide(self, label):
        attrs = [label.get('label'), self.label]
        State.disconnect_collide(attrs)
        self.collide_objects.remove(label.get('name'))
