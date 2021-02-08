#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(digital-daguerreotype VERSION 0.1)

# Add a version number and header file
configure_file(ddConfig.h.in ddConfig.h)
#target_include_directories(digital-daguerreotype PUBLIC "${PROJECT_BINARY_DIR}")

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# add the executable
add_executable(digital-daguerreotype main.cpp)

set_property(TARGET digital-daguerreotype PROPERTY CXX_STANDARD 11)
target_link_libraries(digital-daguerreotype ${DEPENDENCIES})
include_directories(digital-daguerreotype)

#set_target_properties (digital-daguerreotype PROPERTIES FOLDER "Examples")
#install(TARGETS digital-daguerreotype RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
