

target_sources(
        doodle_maya_exe
        PUBLIC

        # AbcExport.h
        AbcWriteJob.h
        AttributesWriter.h
        Foundation.h
        MayaCameraWriter.h
        MayaLocatorWriter.h
        MayaMeshWriter.h
        MayaNurbsCurveWriter.h
        MayaNurbsSurfaceWriter.h
        MayaPointPrimitiveWriter.h
        MayaTransformWriter.h
        MayaUtility.h

        PRIVATE

        # AbcExport.cpp
        MayaUtility.cpp
        MayaTransformWriter.cpp
        AbcWriteJob.cpp
        MayaPointPrimitiveWriter.cpp
        AttributesWriter.cpp
        MayaNurbsSurfaceWriter.cpp
        MayaNurbsCurveWriter.cpp
        MayaMeshWriter.cpp
        MayaCameraWriter.cpp
        MayaLocatorWriter.cpp
)