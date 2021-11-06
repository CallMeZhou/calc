add_executable (echosvc echosvc.cpp)
target_link_libraries (echosvc LINK_PUBLIC com)

add_executable (ascasvc ascasvc.cpp)
target_link_libraries (ascasvc LINK_PUBLIC com)