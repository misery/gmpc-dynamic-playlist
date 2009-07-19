CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

IF(${PROJECT_NAME} STREQUAL "Project")
	MESSAGE(FATAL_ERROR "call parent build file only")
ENDIF()

FIND_PROGRAM(MPD_EXE mpd)
IF(NOT MPD_EXE)
	MESSAGE(FATAL_ERROR "cannot find mpd binary")
ENDIF()

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
ADD_DEFINITIONS(-D MPD_BINARY="\\"${MPD_EXE}\\"")
SET_DIRECTORY_PROPERTIES(PROPERTIES COMPILE_DEFINITIONS G_LOG_DOMAIN="dynlist_tests")

ADD_LIBRARY(fixture SHARED fixture_gmpc.c fixture_mpd.c)
TARGET_LINK_LIBRARIES(fixture ${LIBMPD_LIBRARIES})

MACRO(ADD_TEST_CONFIG target def file)
	FILE(READ ${file}.in config_in)
	STRING(REPLACE "DIRECTORY" "${PROJECT_SOURCE_DIR}/tests" config ${config_in})
	FILE(WRITE ${file} ${config})

	GET_TARGET_PROPERTY(prev ${target} COMPILE_DEFINITIONS)
	SET_TARGET_PROPERTIES(${target} PROPERTIES COMPILE_DEFINITIONS "${def}=\"${file}\";${prev}")
ENDMACRO()

MACRO(ADD_TEST_DATA target test_name)
	ADD_TEST(${test_name} gtester -k --verbose ${target})
	SET_TESTS_PROPERTIES(${test_name} PROPERTIES PASS_REGULAR_EXPRESSION "PASS")
	SET_TESTS_PROPERTIES(${test_name} PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL")

	SET_TARGET_PROPERTIES(${target} PROPERTIES COMPILE_DEFINITIONS "")
	TARGET_LINK_LIBRARIES(${target} dynlist fixture ${DEPS_LIBRARIES} ${LIBMPD_LIBRARIES})
ENDMACRO()

# Played List
ADD_EXECUTABLE(test_played test_played.c)
ADD_TEST_DATA(test_played "Played")

# Blacklist
ADD_EXECUTABLE(test_blacklist test_blacklist.c)
ADD_TEST_DATA(test_blacklist "Blacklist")
ADD_TEST_CONFIG(test_blacklist "CONFIG_EMPTY" "blacklist/empty/config")

#vim:set ts=4 sw=4:
