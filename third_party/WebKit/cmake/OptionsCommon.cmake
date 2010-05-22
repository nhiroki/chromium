ADD_DEFINITIONS(-DBUILDING_WITH_CMAKE=1)
ADD_DEFINITIONS(-DHAVE_CONFIG_H=1)

IF (CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_CXX_FLAGS "-fPIC ${CMAKE_CXX_FLAGS}")
    SET(CMAKE_C_FLAGS "-fPIC ${CMAKE_C_FLAGS}")
ENDIF ()

SET(WTF_INCLUDE_DIRECTORIES
    "${JAVASCRIPTCORE_DIR}"
    "${JAVASCRIPTCORE_DIR}/wtf"
    "${JAVASCRIPTCORE_DIR}/wtf/unicode"
    "${DERIVED_SOURCES_DIR}"
)

IF (WTF_OS_UNIX)
    ADD_DEFINITIONS(-DXP_UNIX)
ENDIF (WTF_OS_UNIX)
