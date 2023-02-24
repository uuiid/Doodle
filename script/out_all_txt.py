import pathlib


class all_file:
    suff_list = ["*.cpp", "*.h", "*.py", "*.re", "*.xml", "*.bat", "*.ps1", "*.txt", ".manifest", "*.cmake", "*.wxi"]

    def __init__(self):
        self.data = ""

    def scanning(self, in_path: pathlib.Path):
        for su in self.suff_list:
            for path in in_path.rglob(su):
                print(path)
                with open(path, encoding="UTF-8") as f:
                    self.data += "file: {}\n".format(path)
                    self.data += f.read().rstrip()

    def out(self):
        with open("F:/doodle.txt", mode="w", encoding="UTF-8") as f:
            f.write(self.data)


if __name__ == '__main__':
    l_txt = all_file()
    l_txt.scanning(pathlib.Path(r"F:\Doodle\src"))
    l_txt.scanning(pathlib.Path(r"F:\Doodle\script"))
    l_txt.scanning(pathlib.Path(r"F:\Doodle\vcpkg"))
    l_txt.out()
