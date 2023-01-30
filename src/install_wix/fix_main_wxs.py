import pathlib
import sys
import tempfile
import uuid
import xml.etree.ElementTree as et
import argparse

et.register_namespace("", "http://wixtoolset.org/schemas/v4/wxs")


class wix_run():

    def __init__(self):
        self.root_path = pathlib.Path(__file__).parent
        self.comm_group: et.Element = None
        self.group_parent: et.Element = None
        self.root_node = et.Element("{http://wixtoolset.org/schemas/v4/wxs}Wix")

    def __get_path_id__(self, path: pathlib.Path):
        if path.suffix == ".exe":
            return path.name.replace(".", "_")
        elif self.root_path == path:
            return str(path.relative_to(self.root_path.parent)) \
                .replace(".", "_") \
                .replace("""\\""", "_") \
                .replace("-", "_")
        else:
            return "id_" + str(hash(path)).replace("-", "_")

    def make_root_xml(self, path: pathlib.Path):
        self.root_path = path
        if not self.root_path.exists():
            self.root_path.mkdir()
        l_fr = et.SubElement(self.root_node, "Fragment")
        l_dir__ref = et.SubElement(l_fr, "DirectoryRef")
        l_dir__ref.attrib["Id"] = "DOODLE_ROOT"
        l_dir_ = et.SubElement(l_dir__ref, "Directory")
        l_dir_.attrib["Id"] = self.__get_path_id__(path)
        l_dir_.attrib["Name"] = path.stem

    def write_xml_file(self, path):
        l_tree = et.ElementTree(self.root_node)
        et.indent(l_tree, space="\t", level=0)
        if not path.parent.exists():
            path.parent.mkdir()

        l_tree.write(path)
        # l_tree.write(path, encoding="utf-8", xml_declaration=True)

    def iter_dir(self):
        self.group_parent = et.SubElement(self.root_node, "Fragment")
        self.comm_group = et.SubElement(self.group_parent, "ComponentGroup")
        self.comm_group.attrib["Id"] = "com_group_" + self.__get_path_id__(self.root_path)
        self.__iter__dir__(self.root_path)

    def __iter__dir__(self, path: pathlib.Path):
        for p in path.iterdir():
            if p.is_dir():
                self.__add_dir__(p)
                self.__iter__dir__(p)
            elif p.is_file():
                self.__add_file__(p)
            else:
                print("未知文件", p)

    def __add_file__(self, path: pathlib.Path):
        if path.is_file() and path.suffix == ".exe":
            l_f = et.SubElement(self.root_node, "Fragment")
            l_com = et.SubElement(l_f, "Component")
        else:
            l_com = et.SubElement(self.comm_group, "Component")
        l_com.attrib["Id"] = "com_" + self.__get_path_id__(path)
        l_com.attrib["Directory"] = self.__get_path_id__(path.parent)
        l_com.attrib["Guid"] = "{{{}}}".format(uuid.uuid4())

        l_file = et.SubElement(l_com, "File")
        l_file.attrib["Id"] = self.__get_path_id__(path)
        l_file.attrib["KeyPath"] = "yes"
        l_file.attrib["Source"] = str(path)

    def __add_dir__(self, path: pathlib.Path):
        l_fr = et.SubElement(self.root_node, "Fragment")
        l_dir__ref = et.SubElement(l_fr, "DirectoryRef")
        l_dir__ref.attrib["Id"] = self.__get_path_id__(path.parent)
        l_dir_ = et.SubElement(l_dir__ref, "Directory")
        l_dir_.attrib["Id"] = self.__get_path_id__(path)
        l_dir_.attrib["Name"] = path.stem

    def __str__(self):
        # xml_tree = et.ElementTree(self.root_node)
        et.indent(self.root_node, "\t", 0)
        return str(et.tostring(self.root_node, encoding="utf-8", xml_declaration=True))
        # with tempfile.TemporaryFile() as tmp_d:
        #     self.write_xml_file(xml_tree, tmp_d)
        #     tmp_d.seek(0)
        #     return str(tmp_d.read())


if __name__ == '__main__':
    parser_ = argparse.ArgumentParser("main_arg")
    parser_.add_argument("--input_dir", help="这是输入的要递归列出的文件夹")
    arg = parser_.parse_args()

    arg_root = pathlib.Path(arg.input_dir)
    wix_run_ = wix_run()
    wix_run_.make_root_xml(arg_root)
    wix_run_.iter_dir()
    # et.dump(wix_run_.root_node)
    wix_run_.write_xml_file(arg_root.parent / "wix" / (arg_root.stem + ".wxs"))
