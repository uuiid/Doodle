

target_sources(
        doodle_maya_lib
        PUBLIC
        alembic_archive_out.h


        # AbcExport.h
        # AbcWriteJob.h
        # AttributesWriter.h
        # Foundation.h
        # MayaCameraWriter.h
        # MayaLocatorWriter.h
        # MayaMeshWriter.h
        # MayaNurbsCurveWriter.h
        # MayaNurbsSurfaceWriter.h
        # MayaPointPrimitiveWriter.h
        # MayaTransformWriter.h
        # MayaUtility.h
        PRIVATE
        alembic_archive_out.cpp


        # # AbcExport.cpp
        # MayaUtility.cpp
        # MayaTransformWriter.cpp
        # AbcWriteJob.cpp
        # MayaPointPrimitiveWriter.cpp
        # AttributesWriter.cpp
        # MayaNurbsSurfaceWriter.cpp
        # MayaNurbsCurveWriter.cpp
        # MayaMeshWriter.cpp
        # MayaCameraWriter.cpp
        # MayaLocatorWriter.cpp
)