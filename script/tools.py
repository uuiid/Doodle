import pymel.core
import datetime
import logging
import logging.handlers


class doodle_log(object):
    log = None

    def __init__(self):
        super(doodle_log, self)
        doodle_log.log = logging.getLogger("doodle")
        format_str = logging.Formatter(
            "%(asctime)s - %(levelname)s - %(message)s")
        doodle_log.log.setLevel(logging.DEBUG)
        sh = logging.StreamHandler()
        sh.setFormatter(format_str)
        th = logging.handlers.TimedRotatingFileHandler(
            filename="E:/tmp/log.txt", when="D", backupCount=3, encoding='utf-8')
        th.setFormatter(format_str)
        doodle_log.log.addHandler(sh)
        doodle_log.log.addHandler(th)


def ref_file(file_path):
    st = datetime.datetime.now()
    pymel.core.openFile(file_path)
    doodle_log.log.info(file_path)
    ed = datetime.datetime.now()
    doodle_log.log.info("open file time : {}".format((ed - st)))

    st = datetime.datetime.now()
    k_lf = pymel.core.listReferences()
    for f in k_lf:
        k_ref = f  # type: pymel.core.FileReference
        doodle_log.log.info(k_ref.path)
        if(k_ref.path == "Q:/6-moxing/Ch/Ch001A/Rig/Ch001A_rig_LX.ma" or
           k_ref.path == "Q:/6-moxing/Ch/Ch001A/Rig/Ch001A_rig_LX_01.ma"
           ):
            k_ref.replaceWith("Q:/6-moxing/Ch/Ch001D/Rig/Ch001D_Rig_LX.ma")
            pymel.core.saveFile(force=True)
    ed = datetime.datetime.now()
    doodle_log.log.info("file replace time : {}".format((ed - st)))


def main():
    with open("E:/tmp/file_list.txt") as f:
        for l in f:
            st = datetime.datetime.now()
            pymel.core.newFile(force=True)
            ref_file(l.rstrip())
            ed = datetime.datetime.now()
            doodle_log.log.info("total time : {}\n".format((ed - st)))


if __name__ == '__main__':
    log = doodle_log()
    main()
