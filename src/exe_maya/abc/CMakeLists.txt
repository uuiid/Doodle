

target_sources(
        doodle_maya_exe
        PUBLIC
        alembic_archive_out.h
        transform_writer.h
        attributes_writer.h

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
        transform_writer.cpp
        attributes_writer.cpp

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