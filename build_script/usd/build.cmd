pip install setuptools
pip install pySide6
pip install PyOpenGL
cd E:\Doodle\external\USD\OpenUSD
python "E:\Doodle\external\USD\OpenUSD\build_scripts\build_usd.py" --embree --openimageio --opencolorio --alembic --hdf5  --materialx --build-variant debug --build ../build ../debug
python "E:\Doodle\external\USD\OpenUSD\build_scripts\build_usd.py" --embree --openimageio --opencolorio --alembic --hdf5  --materialx --build-variant release --build ../build ../release





    The following in your PYTHONPATH environment variable:
    E:\Doodle\external\USD\debug\lib\python

    The following in your PATH environment variable:
    E:\Doodle\external\USD\debug\bin
    E:\Doodle\external\USD\debug\lib