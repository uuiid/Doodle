import argparse


class arg_string:
    def __init__(self):
        try:
            _parser = argparse.ArgumentParser(description="exportMayaFile")
            _parser.add_argument("--path", "-p", help="path and name ")
            _parser.add_argument("--exportpath", "-exp",
                                 help="export path attr")
            _args = _parser.parse_args()
            self.path = _args.path
            self.exportpath = _args.exportpath
        except Exception:
            self.path = ""
            self.exportpath = ""


ARGS = arg_string()


class doodle_log:
    def __init__(self):
        self.log = ""
        self.logPath = ARGS.exportpath

    def write(self):
        if not os.path.exists(self.logPath):
            os.makedirs(self.logPath)
        with open(os.path.join(self.logPath, "doodle_export_abc.json"), "w") as f:
            f.write(
                json.dumps(self.log,
                           ensure_ascii=False,
                           indent=4,
                           separators=(',', ':')
                           )
            )


DLOG = doodle_log()
