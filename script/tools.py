import pymel.core
import datetime


def ref_file(file_path):
    st = datetime.datetime.now()
    pymel.core.openFile(file_path)
    print(file_path)
    ed = datetime.datetime.now()
    print("open file time : {}".format((ed - st)))

    st = datetime.datetime.now()
    k_lf = pymel.core.listReferences()
    for f in k_lf:
        k_ref = f  # type: pymel.core.FileReference
        print(k_ref.path)
        if(k_ref.path == "Q:/6-moxing/Ch/Ch001A/Rig/Ch001A_rig_LX.ma" or
           k_ref.path == "Q:/6-moxing/Ch/Ch001A/Rig/Ch001A_rig_LX_01.ma"
           ):
            k_ref.replaceWith("Q:/6-moxing/Ch/Ch001D/Rig/Ch001D_Rig_LX.ma")
            pymel.core.saveFile(force=True)
    ed = datetime.datetime.now()
    print("file replace time : {}".format((ed - st)))


def main():
    with open("E:/tmp/file_list.txt") as f:
        for l in f:
            st = datetime.datetime.now()
            pymel.core.newFile(force=True)
            ref_file(l.rstrip())
            ed = datetime.datetime.now()
            print("total time : {}\n".format((ed - st)))


if __name__ == '__main__':
    main()
