mkdir build
cd build

cmake -G "Visual Studio 15 2017" -A x64 ^
 -DCMAKE_INSTALL_PREFIX="%CONDA_PREFIX%\Library" ^
 -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
 -DCMAKE_PREFIX_PATH="%CONDA_PREFIX%\Library" ^
 -DCMAKE_SYSTEM_PREFIX_PATH="%CONDA_PREFIX%\Library" ^
 -DPYTHON_EXECUTABLE=python.exe ^
 -DPYTHON_INCLUDE_DIR="%CONDA_PREFIX%\include" ^
 -DPYTHON_LIBRARY="%CONDA_PREFIX%\libs\python37.lib" ^
 -DLIBXML2_LIBRARIES="%CONDA_PREFIX%\Library\lib\libxml2.lib" ^
 -DBOOST_LIBRARYDIR="%CONDA_PREFIX%/Library/lib" ^
 -DBOOST_INCLUDEDIR="%CONDA_PREFIX%/Library/include" ^
 -DOCC_INCLUDE_DIR="%CONDA_PREFIX%\Library\include\opencascade" ^
 -DOCC_LIBRARY_DIR="%CONDA_PREFIX%\Library\lib" ^
 -DCOLLADA_SUPPORT=Off ^
 -DBUILD_IFCPYTHON=off ^
 -DBUILD_EXAMPLES=Off ^
 -DBUILD_GEOMSERVER=Off ^
 -DBUILD_CONVERT=On ^
 ../cmake

cmake --build . --target INSTALL --config RelWithDebInfo
