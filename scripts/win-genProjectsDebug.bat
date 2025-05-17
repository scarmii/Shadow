pushd ..\
cmake CMakeLists.txt -DCMAKE_BUILD_TYPE=Debug -DSH_PROFILE_RENDERER_FUNCTION=ON
cmake .
popd
pause