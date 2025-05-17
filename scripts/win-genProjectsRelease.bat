pushd ..\
cmake CMakeLists.txt -DCMAKE_BUILD_TYPE=Release -DSH_PROFILE_RENDERER_FUNCTION=OFF
cmake .
popd
pause