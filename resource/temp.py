# -*- coding: utf-8 -*-

import pymel.core
import pymel.core.system
import time


def unattr_cam():
    for cam in pymel.core.ls(type='camera', l=True):
        str_l = ["tx", "ty", "tz", "rx", "ry", "rz", "sx", "sy", "sz", "v"]
        trn = cam.getTransform()
        for l_attr in str_l:
            if trn.attr(l_attr).isLocked():
                trn.attr(l_attr).unlock()


def open_ad_save(path, run_fun):
    # type (str, fun)->bool
    pymel.core.system.newFile(force=True)
    pymel.core.system.openFile(path, loadReferenceDepth="none")
    l_t = time.time()
    print("--->Start unlocking {}, time {}".format(path,
          time.asctime(time.localtime(l_t))))
    run_fun()
    print("--->Start saving {}, time {}".format(path, time.time() - l_t))
    pymel.core.system.saveFile(force=True)


if __name__ == '__main__':
    with open("E:/un_cam.txt") as file:
        for line in file:
            l_t = time.time()
            print("---> file open time {}".format(time.asctime(time.localtime(l_t))))
            ls = str(line.rstrip())
            print("--->Start processing {}, time {}".format(ls, time.time() - l_t))
            open_ad_save(ls, unattr_cam)
