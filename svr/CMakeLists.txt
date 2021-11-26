file(GLOB srcs *.cpp)
file(GLOB keypress ${CALC_SOURCE_DIR}/keypress/keypress.c)
add_executable (svr ${srcs} ${keypress})
target_include_directories(svr PUBLIC 
    ${CALC_SOURCE_DIR}/com 
    ${CALC_SOURCE_DIR}/keypress 
    ${CALC_SOURCE_DIR}/stb 
    ${CALC_SOURCE_DIR}/picojson
    ${CALC_SOURCE_DIR}/clipp/include)
target_link_libraries (svr LINK_PUBLIC com pthread fmt::fmt ssl crypto)