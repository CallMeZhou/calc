execute_process(
    COMMAND sh "-c" "ip route get 8.8.8.8 | awk '{print $NF; exit}'"
    OUTPUT_VARIABLE _MYIP
)

string(REGEX REPLACE "\n$" "" _MYIP "${_MYIP}")

configure_file(webgui/index.html webgui/index.html @ONLY)

configure_file(webgui/index.css webgui/index.css COPYONLY)

configure_file(siteconf.json ${CMAKE_BINARY_DIR}/siteconf.json COPYONLY)