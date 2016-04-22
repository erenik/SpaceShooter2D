
include(${CMAKE_SOURCE_DIR}/CMakeMacros.txt)

set(folders ${CMAKE_CURRENT_SOURCE_DIR})
message("Adding folder " ${CMAKE_CURRENT_SOURCE_DIR})
addProject("SpaceShooter2D" "${folders}")


