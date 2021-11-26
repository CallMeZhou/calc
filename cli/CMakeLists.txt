execute_process(
    COMMAND sh "-c" "ip -j route get 8.8.8.8 | jq -r '.[0]|.prefsrc'"
    OUTPUT_VARIABLE _MYIP
)

string(REGEX REPLACE "\n$" "" _MYIP "${_MYIP}")

#configure_file(webgui/index.html webgui/index.html @ONLY)
configure_file(webgui/index.html webgui/index.html COPYONLY)

configure_file(webgui/index.css webgui/index.css COPYONLY)

configure_file(siteconf.json ${CMAKE_BINARY_DIR}/siteconf.json COPYONLY)
configure_file(calc.crt ${CMAKE_BINARY_DIR}/calc.crt COPYONLY)
configure_file(calc.key ${CMAKE_BINARY_DIR}/calc.key COPYONLY)
