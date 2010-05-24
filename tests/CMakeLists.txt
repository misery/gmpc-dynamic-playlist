CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

IF(${PROJECT_NAME} STREQUAL "Project")
	MESSAGE(FATAL_ERROR "call parent build file only")
ENDIF()

INCLUDE(Testing)

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
