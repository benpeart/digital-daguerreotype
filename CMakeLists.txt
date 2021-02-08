#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.10)

# set the project name
project(digital-daguerreotype)

# add the executable
add_executable(${PROJECT_NAME})


# Find librealsense2 installed package
find_package(realsense2 REQUIRED)
if(NOT REALSENSE2_FOUND)
    SET(REALSENSE2_FOUND "realsense2")
    message(WARN "Failed to find_library(realsense2)")
endif()
#target_link_libraries(${PROJECT_NAME} PRIVATE ${realsense2_LIBRARY})


# Find OpenCV, you may need to set OpenCV_DIR variable
# to the absolute path to the directory containing OpenCVConfig.cmake file
# via the command line or GUI
find_package(OpenCV REQUIRED)

# If the package has been found, several variables will
# be set, you can find the full list with descriptions
# in the OpenCVConfig.cmake file.
# Print some message showing some of them
message(STATUS "OpenCV library status:")
message(STATUS "    config: ${OpenCV_DIR}")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")
include_directories(${PROJECT_NAME} ${OpenCV_INCLUDE_DIRS})
#target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})


# Add imgui via source
include_directories(${PROJECT_NAME} ../imgui ../imgui/backends)
target_sources(${PROJECT_NAME} PRIVATE ../imgui/imgui.cpp ../imgui/imgui_draw.cpp ../imgui/imgui_tables.cpp ../imgui/imgui_widgets.cpp ../imgui/backends/imgui_impl_opengl2.cpp ../imgui/backends/imgui_impl_glfw.cpp)


# Add glfw dependences
# from https://www.glfw.org/docs/3.3/build_guide.html#build_link_cmake_package
find_package(glfw3 3.3 REQUIRED)
#target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
find_package(OpenGL REQUIRED)
#target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)


# Add source for digital-daguerreotype
target_sources(${PROJECT_NAME} PRIVATE main.cpp tsp.cpp)


target_link_libraries(${PROJECT_NAME}
    ${realsense2_LIBRARY}
    ${OpenCV_LIBS}
    glfw
    OpenGL::GL
    -lpthread -lm
)


# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# Enable C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)


# Unused/not understood
#set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Examples")
#install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

