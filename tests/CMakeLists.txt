CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

IF(${PROJECT_NAME} STREQUAL "Project")
	MESSAGE(FATAL_ERROR "call parent build file only")
ENDIF()

FIND_PROGRAM(MPD_BIN mpd)
IF(NOT MPD_BIN)
	MESSAGE(FATAL_ERROR "cannot find mpd binary")
ENDIF()

ADD_DEFINITIONS(-D MPD_BINARY="\\"${MPD_BIN}\\"")
SET_DIRECTORY_PROPERTIES(PROPERTIES COMPILE_DEFINITIONS G_LOG_DOMAIN="dynlist_tests")

ADD_LIBRARY(fixture SHARED fixture_gmpc.c fixture_mpd.c)
TARGET_LINK_LIBRARIES(fixture ${DEPS_LIBRARIES} ${LIBMPD_LIBRARIES})

MACRO(ADD_TEST_CONFIG target def file)
	SET(outfile "${PROJECT_BINARY_DIR}/tests/${file}")
	FILE(READ ${file}.in config_in)
	STRING(REPLACE "DIR_SRC" "${PROJECT_SOURCE_DIR}/tests" config ${config_in})
	STRING(REPLACE "DIR_BIN" "${PROJECT_BINARY_DIR}/tests" config ${config})
	FILE(WRITE ${outfile} ${config})

	GET_TARGET_PROPERTY(prev ${target} COMPILE_DEFINITIONS)
	SET_TARGET_PROPERTIES(${target} PROPERTIES COMPILE_DEFINITIONS "${def}=\"${outfile}\";${prev}")
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
ADD_TEST_CONFIG(test_played "CONFIG" "played/config")

# Blacklist
ADD_EXECUTABLE(test_blacklist test_blacklist.c)
ADD_TEST_DATA(test_blacklist "Blacklist")
ADD_TEST_CONFIG(test_blacklist "CONFIG_EMPTY" "blacklist/empty/config")

# Database
ADD_EXECUTABLE(test_database test_database.c)
ADD_TEST_DATA(test_database "Database")
ADD_TEST_CONFIG(test_database "CONFIG_BL_ALL" "database/blacklist/all/config")

#vim:set ts=4 sw=4:
