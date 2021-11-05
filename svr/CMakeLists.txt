file(GLOB cpps *.cpp)
add_executable (svr ${cpps})
target_link_libraries (svr LINK_PUBLIC com)