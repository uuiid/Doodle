import argparse
import os
import json


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

    def write(self, name="doodle.json"):
        if not os.path.exists(self.logPath):
            os.makedirs(self.logPath)
        with open(os.path.join(self.logPath, name), "w") as f:
            f.write(self.log
                    )

            # json.dumps(self.log,
            #            ensure_ascii=False,
            #            indent=4,
            #            separators=(',', ':')
            #            )


DLOG = doodle_log()
