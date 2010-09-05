CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

IF(${PROJECT_NAME} STREQUAL "Project")
	MESSAGE(FATAL_ERROR "call parent build file only")
ENDIF()

INCLUDE(Testing)

# Fixture
ADD_EXECUTABLE(test_fixture test_fixture.c)
ADD_TEST_DATA(test_fixture "Fixture")
ADD_TEST_CONFIG(test_fixture "CONFIG" "played/config")

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

# Prune
ADD_EXECUTABLE(test_prune test_prune.c)
ADD_TEST_DATA(test_prune "Pruning")
ADD_TEST_CONFIG(test_prune "CONFIG" "played/config")

# Prefs
ADD_EXECUTABLE(test_prefs test_prefs.c)
ADD_TEST_DATA(test_prefs "Preferences")

# dbSong
ADD_EXECUTABLE(test_dbSong test_dbSong.c)
ADD_TEST_DATA(test_dbSong "dbSong")

# Icon
ADD_EXECUTABLE(test_icon test_icon.c)
ADD_TEST_DATA(test_icon "Icon")

#vim:set ts=4 sw=4:
