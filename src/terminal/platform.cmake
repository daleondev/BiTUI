if(WIN32)
    target_sources(
        BiTUI
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/Win32TerminalBackend.cpp
    )
elseif(UNIX)
    target_sources(
        BiTUI
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/PosixTerminalBackend.cpp
    )
else()
    message(FATAL_ERROR "BiTUI does not have a terminal backend for this platform yet")
endif()