# CMakeList.txt : CMake project for Fractal, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Fractal "Fractal.cpp" "Fractal.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Fractal PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
