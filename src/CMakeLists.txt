cmake_minimum_required(VERSION 3.10)

# set the project name
project(MajorHelper)

# Setup c++ vars
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Libs
find_package(FLTK REQUIRED)
find_package(OpenGL REQUIRED)



# FLTK wrapping
# fltk_wrap_ui(Ui ui.fl)
# add_library(Ui callbacks.cxx ${Ui_FLTK_UI_SRCS})

# add the executable
add_executable(MajorHelper major_helper.cxx)
# add_dependencies(MajorHelper Ui)
target_link_libraries(MajorHelper fltk)
