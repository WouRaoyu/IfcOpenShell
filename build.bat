mkdir build
cd build

cmake -G "Visual Studio 15 2017" -A x64 ^
 -DCMAKE_INSTALL_PREFIX="%CONDA_PREFIX%\Library" ^
 -DCMAKE_TOOLCHAIN_FILE=%CONDA_PREFIX%\h2o.cmake ^
 -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
 -DCMAKE_PREFIX_PATH="%CONDA_PREFIX%\Library" ^
 -DCMAKE_SYSTEM_PREFIX_PATH="%CONDA_PREFIX%\Library" ^
 -DLIBXML2_LIBRARIES="%CONDA_PREFIX%\Library\lib\libxml2.lib" ^
 -DOCC_INCLUDE_DIR="%CONDA_PREFIX%\Library\include\opencascade" ^
 -DOCC_LIBRARY_DIR="%CONDA_PREFIX%\Library\lib" ^
 -DJSON_INCLUDE_DIR="%CONDA_PREFIX%\Library\include" ^
 -DCOLLADA_SUPPORT=Off ^
 -DBUILD_IFCPYTHON=off ^
 -DBUILD_EXAMPLES=On ^
 -DBUILD_GEOMSERVER=Off ^
 -DBUILD_CONVERT=On ^
 -DGLTF_SUPPORT=On ^
 ../cmake

REM cmake --build . --target INSTALL --config RelWithDebInfo

cd ..