def select_ctrl():
    import pymel.core
    l_select = pymel.core.ls("*_ctrl")
    pymel.core.select(l_select)


select_ctrl()


def to_default():
    import pymel.core
    l_select = pymel.core.ls(sl=True)
    for c in l_select:
        for attr_n in pymel.core.listAttr(c):
            attr = c.attr(attr_n)
            if attr.isKeyable():
                l_def = pymel.core.attributeQuery(
                    attr_n, node=c, listDefault=True)[0]
                attr.set(l_def)


to_default()


"""



1. 重命名(找到重名)
2. 控制器默认值(先)
3. 蒙皮(model)骨骼是否在组内(show)
4. 是否有组(model)
"""
