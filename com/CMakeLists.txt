file(GLOB cpps *.cpp)
add_library (com ${cpps})
target_include_directories (com PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CALC_SOURCE_DIR}/fmt/include)